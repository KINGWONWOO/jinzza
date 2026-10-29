# JINZZA 캐릭터 디자인 프롬프트 (v2 — 필터 차단 회피용 완화 버전)

**v2에서 바뀐 점**: "character reference sheet / mascot / video game / T-pose / orthographic"
같은 상업적·기술적 용어들이 일부 생성기에서 IP/브랜드 안전 필터를 과민하게 작동시킬 수 있어서,
순수하게 그림을 묘사하는 문장으로 다시 썼어. 그래도 막히면 맨 아래 "그래도 막힐 때" 섹션을 봐줘.

컨셉 조건: 약간 멍청해/순박해 보이는 표정 + 사람처럼 가까이서 눈을 들여다보는 느낌(큰 눈, 또렷한 홍채/눈동자, 감정이 읽히는 눈). 게임 특성(판정단이 얼굴/눈을 관찰해서 진짜를 찾는다)과 잘 맞물리는 방향이야. 세 가지 종 옵션을 준비했으니 결과물 비교해보고 골라줘.

공통 요구사항(세 프롬프트 모두 반영됨):
- 손(손가락)이 있어야 함 — 이모트/소품 들기 때문에 필수
- 사람 체형 비율(두 발로 서는 이족보행, SD/치비 느낌으로 머리를 살짝 크게)
- 나중에 모듈로 갈아끼울 걸 전제로, 머리는 맨머리(헤어는 얹는 형태), 얼굴은 액세서리 없이 깨끗한 베이스

---

## 옵션 A — 사람 베이스 (제일 안전한 선택)

```
A cute digital illustration of a short, slightly chubby humanoid character, standing facing
forward with arms relaxed at the sides, on a plain white background. Chibi proportions with a big
round head and a small simple body. Smooth round bald head with no hair, plain clean face with no
accessories. A slightly dumb, goofy, naive facial expression — mouth slightly open in a simple
clueless smile, one eyebrow raised a little. The eyes are the focal point: large, round, highly
detailed and expressive, with a visible iris, pupil, and a small catchlight highlight, as if
looking directly at the viewer up close like a real person's face. Simple five-fingered hands,
wearing a plain neutral-colored jumpsuit with no logos or text. Flat cel-shaded illustration
style, clean bold outlines, soft toon shading, no gradients, no realistic skin texture, simple
flat background color.
```

---

## 옵션 B — 동물 마스코트 (곰/너구리/여우 등, 손 있는 이족보행)

```
A cute digital illustration of a small bear character standing upright on two legs facing
forward, arms relaxed at the sides, on a plain white background. Chibi proportions with a big
round head and small simple body. Two arms ending in simple five-fingered hands capable of holding
objects. Round plain head with short simple fur texture, no hairstyle, no hat, no accessories on
the face. A slightly dumb, goofy, derpy facial expression — mouth slightly open, a little
slack-jawed, endearing and silly. The eyes are the focal point: large, round, highly detailed and
expressive, with a visible iris, pupil, and catchlight, as if looking directly at the viewer up
close like a real person's face rather than simple dot eyes. Wearing a plain neutral-colored vest
with no logos or text. Flat cel-shaded illustration style, clean bold outlines, soft toon shading,
no gradients, simple flat background color.
```

*(곰 대신 너구리/여우/토끼로 바꾸고 싶으면 "small bear character"만 "small raccoon character" 등으로 교체)*

---

## 옵션 C — 물범을 포기하지 않는다면 (손 달린 이족보행 물범으로 재설계)

```
A cute digital illustration of a small seal character redesigned to stand upright on two short
legs facing forward, with two short arms ending in simple five-fingered hands (not flippers)
capable of holding objects, on a plain white background. Chibi proportions with an oversized
round head and small round body. Smooth plain head, no hairstyle, no accessories. A slightly
dumb, goofy, derpy facial expression — mouth slightly open in a clueless simple smile. The eyes
are the focal point: large, round, highly detailed and expressive, with a visible iris, pupil,
and catchlight, as if looking directly at the viewer up close like a real person's face. Wearing
a plain neutral-colored outfit with no logos or text. Flat cel-shaded illustration style, clean
bold outlines, soft toon shading, no gradients, simple flat background color.
```

---

## 추가로 뽑아볼 만한 것 — 눈 클로즈업 표정 시트

방향 하나를 고른 뒤, 그 캐릭터의 "관찰당하는 느낌"이 실제로 잘 사는지 확인하려면 클로즈업 표정 시트도 하나 뽑아보는 걸 추천해:

```
A close-up front-facing portrait of [여기에 고른 캐릭터 설명 붙여넣기], showing 4 different facial
expressions in a grid: neutral/blank, nervous/sweating, confidently smiling, and surprised/caught.
Focus heavily on the eyes in every expression — large, detailed, human-like, expressive, as if the
viewer is studying this face up close trying to tell if something is hidden behind it. Flat
cel-shaded stylized game art style, clean outlines, plain white background, no text or labels.
```

---

## 사용 팁

- 세 옵션 다 뽑아서 비교해보고, 그중 "멍청해 보이는 정도"와 "눈이 사람처럼 느껴지는 정도"가 제일 맘에 드는 걸 베이스로 확정하는 걸 추천해.
- 베이스가 정해지면 그 캐릭터 설명을 고정 문구로 재사용해서 "헤어스타일 A를 쓴 버전", "액세서리 B를 쓴 버전"처럼 변형만 바꿔가며 더 뽑으면, 지금 만들어둔 조합형 커스터마이징(헤어/눈썹/액세서리 교체) 콘셉트와도 자연스럽게 이어져.
- 옵션 B/C처럼 동물로 갈 경우, 실제 3D 모델링 단계에서는 반드시 **사람형 스켈레톤과 호환되는 뼈 구조로 리깅**해야 지금까지 짜놓은 애니메이션/소켓 커스터마이징 코드를 그대로 재사용할 수 있어 — 모델러나 애셋팩을 구할 때 이 점을 꼭 조건으로 걸어줘.
- 참고용으로 모델러에게 넘길 정면/측면 캐릭터 시트가 필요하면, 일단 위 프롬프트로 정면 이미지를 먼저 뽑은 다음 "지금 그린 캐릭터의 측면(side profile) 모습을 같은 스타일로 그려줘"처럼 **후속 요청으로 따로** 받는 걸 추천해 — "reference sheet / T-pose / orthographic" 같은 기술 용어를 한 프롬프트에 몰아넣는 것보다 필터에 덜 걸려.

## 그래도 차단될 때

- **어떤 옵션에서 막혔는지, 어떤 생성 도구를 썼는지** 알려줘 — 그 조합에 맞춰 더 좁혀서 다시 써줄 수 있어.
- 문장을 통째로 바꾸지 말고 **한 구간씩 지워보면서** 테스트해봐 (예: "다섯 손가락이 있는 손" 부분을 빼고 시도 → 됐다가 다시 걸리면 그 표현이 원인). 대부분 특정 단어 하나가 과민하게 걸리는 경우가 많아서, 범인만 찾으면 의미는 유지한 채 다른 말로 바꿀 수 있어.
- 같은 프롬프트를 다른 생성기(예: ChatGPT의 이미지 생성, Midjourney, Grok 등)에 넣어보는 것도 방법이야 — 필터 기준이 도구마다 달라서 한 곳에서 막혀도 다른 곳에서는 통과되는 경우가 흔해.
