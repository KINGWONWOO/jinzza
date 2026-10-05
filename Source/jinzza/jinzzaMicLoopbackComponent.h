// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "AudioCaptureCore.h"
#include <atomic>
#include "jinzzaMicLoopbackComponent.generated.h"

/**
 * Plays a microphone straight back out (local loopback) - the Voice Test's "hear yourself"
 * source. Replaces UAudioCaptureComponent there because that one can only open the system default
 * mic, plays it back at raw mic level, and can only change pitch by resampling (which changes
 * playback SPEED - the capture buffer then runs dry or piles up, and every pitch change needed a
 * Stop/Start that cut the voice out):
 *  - opens any capture device by index (DeviceIndex, INDEX_NONE = system default) via
 *    Audio::FAudioCapture;
 *  - mixes all input channels to mono by SUMMING them (some USB mics put the voice on only one
 *    channel of a stereo stream - averaging would halve it);
 *  - processing per sample: 80 Hz high-pass (rumble/DC) -> Gain ("mic boost") -> real-time pitch
 *    shifter (two crossfaded delay-line taps; speed stays real-time, so SetPitch is live and
 *    glitch-free) -> limiter that only touches peaks above -3 dBFS;
 *  - keeps a small cushion of buffered audio (re-primes after an underrun instead of crackling,
 *    and drops audio if latency builds up past ~0.1s);
 *  - exposes the live input level (GetInputLevel, 0-1 peak, after gain) for a level meter.
 * It's a USynthComponent, so SourceEffectChain (robot/echo) still applies on top. Set DeviceIndex
 * before the first Start(); to switch devices, destroy it and make a new one (the stream is
 * opened once, in Init).
 */
UCLASS(ClassGroup = Synth)
class JINZZA_API UjinzzaMicLoopbackComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UjinzzaMicLoopbackComponent(const FObjectInitializer& ObjectInitializer);

	/** Capture device index into Audio::FAudioCapture::GetCaptureDevicesAvailable, or INDEX_NONE for the system default. */
	int32 DeviceIndex = INDEX_NONE;

	/** Linear gain applied before the pitch shifter/limiter. Live. */
	void SetGain(float InGain) { Gain.store(FMath::Max(0.f, InGain)); }

	/** Pitch ratio (1 = unchanged, 2 = an octave up, 0.5 = an octave down). Live, no restart needed. */
	void SetPitch(float InPitch) { Pitch.store(FMath::Clamp(InPitch, 0.5f, 2.f)); }

	/** Latest input peak (0-1, after gain, before the limiter), for a UI level meter. */
	float GetInputLevel() const { return InputLevel.load(); }

	/** The opened device's name, or empty if none could be opened. */
	const FString& GetOpenedDeviceName() const { return OpenedDeviceName; }

	virtual void BeginDestroy() override;

protected:
	//~ Begin USynthComponent interface
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual void OnBeginGenerate() override;
	virtual void OnEndGenerate() override;
	//~ End USynthComponent interface

private:
	void CloseCapture();

	/** High-pass + gain + pitch shift + limiter for one sample (audio render thread). */
	float ProcessSample(float Input, float CurrentGain, float CurrentPitch);

	Audio::FAudioCapture AudioCapture;

	/** Mono samples from the capture callback, consumed by OnGenerateAudio. */
	FCriticalSection BufferLock;
	TArray<float> PendingSamples;

	/** Read buffer on the audio render thread (avoids allocating inside the lock). */
	TArray<float> ReadScratch;

	std::atomic<float> Gain{ 3.f };
	std::atomic<float> Pitch{ 1.f };
	std::atomic<float> InputLevel{ 0.f };
	std::atomic<bool> bGenerating{ false };
	bool bPrimed = false;
	bool bStreamOpen = false;
	FString OpenedDeviceName;

	// Buffering targets (samples), set from the device sample rate in Init.
	int32 PrimeSamples = 2048;
	int32 MaxLatencySamples = 6144;

	// --- DSP state (audio render thread only) ---
	float SampleRateHz = 48000.f;
	float HighPassCoeff = 0.99f;
	float HighPassPrevIn = 0.f;
	float HighPassPrevOut = 0.f;

	static constexpr int32 ShiftBufferSize = 8192; // power of two
	TArray<float> ShiftBuffer;
	int32 ShiftWritePos = 0;
	float ShiftPhase = 0.f;
	float ShiftWindow = 2400.f;
	/** 0 = dry (pitch 1), 1 = fully shifted - ramps so toggling the shifter on/off doesn't click. */
	float ShiftMix = 0.f;
};
