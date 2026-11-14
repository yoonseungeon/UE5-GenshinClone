// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/PlayableAnimInstance.h"
#include "GameFramework/Pawn.h"

// [애니메이션-02] Run Stop 진입 시 마지막 평가 커브로 RunStop or RunStop+Mirror를 선택한다.
void UPlayableAnimInstance::SelectRunStopVariant()
{
	static const FName CurveName(TEXT("RunStopVariant"));
	float Value = 0.0f;
	// Value: 1 or -1
	bUseMirroredRunStop = GetCurveValue(CurveName, Value) && FMath::IsFinite(Value) && Value > 0.0f;
}

// [애니메이션-03] Run 진입 시 마지막 RunStop 동작으로 달리기 동작을 선택한다.
void UPlayableAnimInstance::SelectRunVariant()
{
	bUseMirroredRun = !bUseMirroredRunStop;
}

// [애니메이션-01] 애니메이션이 업데이트될 때마다 호출된다.
void UPlayableAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APawn* Pawn = TryGetPawnOwner();
	// 애니메이션 갱신 시 이동 입력 유무를 판정한다.
	bShouldRun = IsValid(Pawn) && !Pawn->GetLastMovementInputVector().IsNearlyZero();
}
