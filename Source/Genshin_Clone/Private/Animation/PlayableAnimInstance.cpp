// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/PlayableAnimInstance.h"

#include "GameFramework/Pawn.h"
#include "Characters/PlayableCharacter.h"

#include "Animation/AnimSequence.h"

// [애니메이션-01] 애니메이션이 업데이트될 때마다 호출된다.
// [대시-10] 게임 스레드에서 사용자 입력과 대시 데이터를 AnimGraph용 스냅샷으로 복사한다.
void UPlayableAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APlayableCharacter* Character = Cast<APlayableCharacter>(TryGetPawnOwner());
	if (!Character)
	{
		bShouldRun = false;
		DashState = FDashRuntimeState{};
		DashStartAnimation = nullptr;
		DashStopAnimation = nullptr;
		DashReturnAnimation = nullptr;

		RunStartPosition = 0.0f;
		bDashReturnUsesMirroredRun = false;
		LastConsumedDashActivationId = 0;

		return;
	}

	// 애니메이션 갱신 시 이동 입력 유무를 판정한다.
	bShouldRun = !Character->GetMovementIntent().IsNearlyZero();

	UDashComponent* DashComponent = Character->GetDashComponent();
	DashState = DashComponent->GetRuntimeState();
	DashStartAnimation = DashComponent->GetAnimationForPhase(EDashPhase::Starting);
	DashStopAnimation = DashComponent->GetAnimationForPhase(EDashPhase::Stopping);
	DashReturnAnimation = DashComponent->GetAnimationForPhase(EDashPhase::ReturningToRun);

	// 다음 Run을 원본·미러 중 무엇으로, 몇 초부터 재생할지 미리 준비하는 코드
	const FDashAnimationSet& AnimationSet = DashComponent->GetActiveAnimationSet();
	bDashReturnUsesMirroredRun = AnimationSet.bUseMirroredRun;
	const bool bHasNewDashReturn = DashState.EndReason == EDashEndReason::ReturnedToRun && DashState.ActivationId != LastConsumedDashActivationId;
	RunStartPosition = bHasNewDashReturn ? FMath::Max(AnimationSet.RunPoseMatchTime - DashComponent->GetReturnToRunBlendTime(), 0.0f) : 0.0f;
}

// [애니메이션-03] Run 진입 시 마지막 RunStop 동작으로 달리기 동작을 선택한다.
// [대시-11] 일반 Run 진입은 RunStop 기준으로, 정상 대시 복귀는 세트 기준으로 한 번 선택한다.
void UPlayableAnimInstance::SelectRunVariant()
{
	const bool bHasNewDashReturn = DashState.EndReason == EDashEndReason::ReturnedToRun && DashState.ActivationId != LastConsumedDashActivationId;
	bUseMirroredRun = bHasNewDashReturn ? bDashReturnUsesMirroredRun : !bUseMirroredRunStop;
	LastConsumedDashActivationId = DashState.ActivationId;
}

// [대시-11] SprintStop의 원본·미러 선택을 공유하는 정지 기억에 저장한다.
void UPlayableAnimInstance::RememberDashStopVariant()
{
	bUseMirroredRunStop = DashState.Variant == EDashAnimationVariant::Mirrored;
}

// [대시-11] 커브로 대시 변형을 선택하고, 사용할 값이 없으면 정지 기억의 반대를 선택한다.
EDashAnimationVariant UPlayableAnimInstance::GetDashStartVariant() const
{
	static const FName CurveName(TEXT("RunStopVariant"));
	float Value = 0.0f;
	// Stanby 재생 중 일 때 RunCycle Curve니깐 GetCurveValue가 false 반환함.
	if (GetCurveValue(CurveName, Value) && FMath::IsFinite(Value) && !FMath::IsNearlyZero(Value))
		return Value > 0.0f ? EDashAnimationVariant::Mirrored : EDashAnimationVariant::Normal;

	return bUseMirroredRunStop ? EDashAnimationVariant::Normal : EDashAnimationVariant::Mirrored;
}

// [애니메이션-02] RunStop 진입 시 마지막 평가 커브로 정지 클립의 원본·미러를 선택한다.
void UPlayableAnimInstance::SelectRunStopVariant()
{
	static const FName CurveName(TEXT("RunStopVariant"));
	float Value = 0.0f;
	// Value: 1 or -1
	bUseMirroredRunStop = GetCurveValue(CurveName, Value) && FMath::IsFinite(Value) && Value > 0.0f;
}
