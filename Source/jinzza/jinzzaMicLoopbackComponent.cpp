// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMicLoopbackComponent.h"
#include "jinzza.h"

namespace
{
	/** Peaks above this (linear, ~-3 dBFS) are softly squashed; everything below passes untouched. */
	constexpr float LimiterThreshold = 0.7f;

	/** Pitch ratios this close to 1 bypass the shifter (a shifter at ratio 1 still comb-filters slightly). */
	constexpr float PitchBypassTolerance = 0.01f;
}

UjinzzaMicLoopbackComponent::UjinzzaMicLoopbackComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Output is always mono (the input channels are summed in the capture callback).
	NumChannels = 1;
}

bool UjinzzaMicLoopbackComponent::Init(int32& SampleRate)
{
	NumChannels = 1;

	Audio::FCaptureDeviceInfo DeviceInfo;
	if (!AudioCapture.GetCaptureDeviceInfo(DeviceInfo, DeviceIndex))
	{
		UE_LOG(Logjinzza, Warning, TEXT("MicLoopback: no capture device at index %d."), DeviceIndex);
		return false;
	}

	if (DeviceInfo.PreferredSampleRate > 0)
	{
		SampleRate = DeviceInfo.PreferredSampleRate;
	}
	SampleRateHz = static_cast<float>(SampleRate);

	// ~40 ms cushion before playback (at least two capture blocks, so bursty delivery doesn't
	// underrun), and never more than ~0.1 s behind live.
	PrimeSamples = FMath::Max(2048, FMath::RoundToInt(SampleRateHz * 0.04f));
	MaxLatencySamples = PrimeSamples + FMath::RoundToInt(SampleRateHz * 0.08f);

	// 80 Hz one-pole high-pass.
	const float RC = 1.f / (2.f * PI * 80.f);
	const float Dt = 1.f / SampleRateHz;
	HighPassCoeff = RC / (RC + Dt);

	// 50 ms pitch-shifter window - long enough for voice pitch periods, short enough not to smear words.
	ShiftWindow = SampleRateHz * 0.05f;
	ShiftBuffer.SetNumZeroed(ShiftBufferSize);

	Audio::FAudioCaptureDeviceParams Params;
	Params.DeviceIndex = DeviceIndex;

	// Called on the capture device's thread: sum the interleaved channels to mono and queue them.
	Audio::FOnAudioCaptureFunction OnCapture = [this](const void* AudioData, int32 NumFrames, int32 InNumChannels, int32 InSampleRate, double StreamTime, bool bOverFlow)
	{
		if (!bGenerating.load() || InNumChannels <= 0)
		{
			return;
		}

		const float* Input = static_cast<const float*>(AudioData);
		const float CurrentGain = Gain.load();
		float Peak = 0.f;

		FScopeLock Lock(&BufferLock);
		const int32 Start = PendingSamples.AddUninitialized(NumFrames);
		float* Out = PendingSamples.GetData() + Start;
		for (int32 Frame = 0; Frame < NumFrames; ++Frame)
		{
			float Sum = 0.f;
			for (int32 Channel = 0; Channel < InNumChannels; ++Channel)
			{
				Sum += Input[Frame * InNumChannels + Channel];
			}
			Out[Frame] = Sum;
			Peak = FMath::Max(Peak, FMath::Abs(Sum) * CurrentGain);
		}

		// Latency cap: if playback has fallen behind, jump forward to just the cushion.
		if (PendingSamples.Num() > MaxLatencySamples)
		{
			PendingSamples.RemoveAt(0, PendingSamples.Num() - PrimeSamples, EAllowShrinking::No);
		}

		InputLevel.store(FMath::Min(Peak, 1.f));
	};

	if (!AudioCapture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024))
	{
		UE_LOG(Logjinzza, Warning, TEXT("MicLoopback: failed to open capture device '%s'."), *DeviceInfo.DeviceName);
		return false;
	}

	// Started here, on the game thread, like UAudioCaptureComponent does - starting it from the
	// audio render thread can hitch.
	if (!AudioCapture.StartStream())
	{
		UE_LOG(Logjinzza, Warning, TEXT("MicLoopback: failed to start capture device '%s'."), *DeviceInfo.DeviceName);
		AudioCapture.CloseStream();
		return false;
	}

	bStreamOpen = true;
	OpenedDeviceName = DeviceInfo.DeviceName;
	PendingSamples.Reserve(MaxLatencySamples + 4096);
	ReadScratch.Reserve(MaxLatencySamples + 4096);
	UE_LOG(Logjinzza, Log, TEXT("MicLoopback: opened '%s' (%d ch, %d Hz)."), *DeviceInfo.DeviceName, DeviceInfo.InputChannels, SampleRate);
	return true;
}

void UjinzzaMicLoopbackComponent::OnBeginGenerate()
{
	{
		FScopeLock Lock(&BufferLock);
		PendingSamples.Reset();
	}
	bPrimed = false;
	HighPassPrevIn = 0.f;
	HighPassPrevOut = 0.f;
	FMemory::Memzero(ShiftBuffer.GetData(), ShiftBuffer.Num() * sizeof(float));
	ShiftWritePos = 0;
	ShiftPhase = 0.f;
	bGenerating.store(true);
}

void UjinzzaMicLoopbackComponent::OnEndGenerate()
{
	bGenerating.store(false);
	InputLevel.store(0.f);
}

float UjinzzaMicLoopbackComponent::ProcessSample(float Input, float CurrentGain, float CurrentPitch)
{
	// 1. High-pass: strips DC offset and low rumble/handling noise that just muddies the voice.
	const float HighPassed = HighPassCoeff * (HighPassPrevOut + Input - HighPassPrevIn);
	HighPassPrevIn = Input;
	HighPassPrevOut = HighPassed;

	// 2. Gain.
	const float Boosted = HighPassed * CurrentGain;

	// 3. Pitch shift: two read taps sweep through a delay line at (1 - pitch) relative speed,
	// half a window apart, each faded in/out with sin^2 so one is always at full volume while
	// the other jumps back. Playback speed stays real-time.
	const int32 Mask = ShiftBufferSize - 1;
	ShiftBuffer[ShiftWritePos & Mask] = Boosted;

	const float TargetMix = FMath::Abs(CurrentPitch - 1.f) < PitchBypassTolerance ? 0.f : 1.f;
	ShiftMix += (TargetMix - ShiftMix) * 0.002f; // ~10 ms ramp at 48 kHz

	float Output = Boosted;
	if (ShiftMix > 0.0001f)
	{
		ShiftPhase += (1.f - CurrentPitch) / ShiftWindow;
		ShiftPhase -= FMath::FloorToFloat(ShiftPhase);

		auto ReadTap = [this, Mask](float Phase) -> float
		{
			const float Delay = Phase * ShiftWindow + 1.f;
			const float ReadPos = static_cast<float>(ShiftWritePos) - Delay;
			const int32 Index = FMath::FloorToInt(ReadPos);
			const float Frac = ReadPos - static_cast<float>(Index);
			return FMath::Lerp(ShiftBuffer[Index & Mask], ShiftBuffer[(Index + 1) & Mask], Frac);
		};

		const float PhaseB = FMath::Frac(ShiftPhase + 0.5f);
		const float FadeA = FMath::Square(FMath::Sin(PI * ShiftPhase));
		const float Shifted = FadeA * ReadTap(ShiftPhase) + (1.f - FadeA) * ReadTap(PhaseB);
		Output = FMath::Lerp(Boosted, Shifted, ShiftMix);
	}
	++ShiftWritePos;

	// 4. Limiter: linear below LimiterThreshold, smooth tanh knee above it (never exceeds 1).
	const float Magnitude = FMath::Abs(Output);
	if (Magnitude > LimiterThreshold)
	{
		const float Headroom = 1.f - LimiterThreshold;
		const float Squashed = LimiterThreshold + Headroom * FMath::Tanh((Magnitude - LimiterThreshold) / Headroom);
		Output = FMath::Sign(Output) * Squashed;
	}
	return Output;
}

int32 UjinzzaMicLoopbackComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	FMemory::Memzero(OutAudio, NumSamples * sizeof(float));
	if (!bStreamOpen)
	{
		return NumSamples;
	}

	ReadScratch.Reset();
	{
		FScopeLock Lock(&BufferLock);
		if (!bPrimed)
		{
			if (PendingSamples.Num() < PrimeSamples)
			{
				return NumSamples;
			}
			bPrimed = true;
		}

		const int32 NumToRead = FMath::Min(NumSamples, PendingSamples.Num());
		ReadScratch.Append(PendingSamples.GetData(), NumToRead);
		PendingSamples.RemoveAt(0, NumToRead, EAllowShrinking::No);

		// Underrun: rather than crackling block after block, wait for a fresh cushion.
		if (NumToRead < NumSamples)
		{
			bPrimed = false;
		}
	}

	const float CurrentGain = Gain.load();
	const float CurrentPitch = Pitch.load();
	for (int32 Index = 0; Index < ReadScratch.Num(); ++Index)
	{
		OutAudio[Index] = ProcessSample(ReadScratch[Index], CurrentGain, CurrentPitch);
	}
	return NumSamples;
}

void UjinzzaMicLoopbackComponent::CloseCapture()
{
	bGenerating.store(false);
	if (bStreamOpen)
	{
		AudioCapture.StopStream();
		AudioCapture.CloseStream();
		bStreamOpen = false;
	}
}

void UjinzzaMicLoopbackComponent::BeginDestroy()
{
	// Close before the capture callback (which captures `this`) could fire on a dying object.
	CloseCapture();
	Super::BeginDestroy();
}
