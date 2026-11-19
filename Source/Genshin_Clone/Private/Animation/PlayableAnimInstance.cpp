// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/PlayableAnimInstance.h"

#include "GameFramework/Pawn.h"
#include "Characters/PlayableCharacter.h"

#include "SequencePlayerLibrary.h"

// [애니메이션-01] 애니메이션이 업데이트될 때마다 호출된다.
// [대시-16] 이동 입력·정지 클립 기억·RunCycle 복귀 시간을 AnimGraph용으로 준비한다.
void UPlayableAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(TryGetPawnOwner());
	if (!PlayableCharacter)
		return;

	bHasMovementInput = !PlayableCharacter->GetMovementInputDirection().IsNearlyZero();
	UDashComponent* DashComponent = PlayableCharacter->GetDashComponent();

	// 새 SprintStop 재생 이력을 공유 정지 기억에 반영한다.
	const int32 SprintStopDashId = DashComponent->GetLastSprintStopDashId();
	if (SprintStopDashId != 0 && SprintStopDashId != LastRecordedSprintStopDashId)
	{
		bNextRunStopIsMirrored = DashComponent->GetLastSprintStopVariant() == EDashAnimationVariant::Mirrored;
		LastRecordedSprintStopDashId = SprintStopDashId;
	}

	DashStateSnapshot = DashComponent->GetDashRuntimeState();
	bRunCycleNeedsTimeSyncAfterDash = (DashStateSnapshot.EndReason == EDashEndReason::ReturnedToRun) && (DashStateSnapshot.DashId != LastRunCycleReturnDashId);

	// 이동 입력이 없으면 이번 복귀 시간 맞춤을 건너뛰어 이후 일반 RunCycle에 남기지 않는다.
	if (bRunCycleNeedsTimeSyncAfterDash && !bHasMovementInput)
	{
		LastRunCycleReturnDashId = DashStateSnapshot.DashId;
		bRunCycleNeedsTimeSyncAfterDash = false;
	}

	RunCycleStartTimeSeconds = 0.0f;
	if (bRunCycleNeedsTimeSyncAfterDash)
	{
		bNextRunCycleIsMirrored = DashComponent->GetCurrentDashAnimationSet().bRunCycleIsMirrored;
		RunCycleStartTimeSeconds = DashComponent->GetRunCycleStartTimeAfterDash();
	}
}

// [애니메이션-03] 일반 RunCycle 진입은 마지막 정지 클립의 반대 변형을 선택한다.
void UPlayableAnimInstance::SelectRunCycleClip()
{
	if (bRunCycleNeedsTimeSyncAfterDash)
		return;

	bNextRunCycleIsMirrored = !bNextRunStopIsMirrored;
}

// [대시-17] 선택된 RunCycle 노드의 재생 시간을 맞추고 완료한 대시 번호를 저장한다.
void UPlayableAnimInstance::SetRunCycleTimeAfterDash(const FAnimNodeReference& RunCycleNode, bool bRunCycleNodeIsMirrored)
{
	if (!bRunCycleNeedsTimeSyncAfterDash || bRunCycleNodeIsMirrored != bNextRunCycleIsMirrored)
		return;

	EAnimNodeReferenceConversionResult NodeConversionResult;
	const FSequencePlayerReference RunCyclePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(RunCycleNode, NodeConversionResult);
	if (NodeConversionResult != EAnimNodeReferenceConversionResult::Succeeded)
		return;

	USequencePlayerLibrary::SetAccumulatedTime(RunCyclePlayer, RunCycleStartTimeSeconds);
	LastRunCycleReturnDashId = DashStateSnapshot.DashId;
	bRunCycleNeedsTimeSyncAfterDash = false;
}

// [대시-18] RunStopVariant 커브의 부호로 대시 변형을 고르고, 커브가 없으면 마지막 정지 클립의 반대를 고른다.
EDashAnimationVariant UPlayableAnimInstance::GetDashStartVariant() const
{
	static const FName RunStopVariantCurveName(TEXT("RunStopVariant"));
	float RunStopVariantCurveValue = 0.0f;
	// Standby처럼 RunStopVariant 커브가 없는 포즈에서는 GetCurveValue가 false를 반환한다.
	if (GetCurveValue(RunStopVariantCurveName, RunStopVariantCurveValue))
		return RunStopVariantCurveValue > 0.0f ? EDashAnimationVariant::Mirrored : EDashAnimationVariant::Normal;

	return bNextRunStopIsMirrored ? EDashAnimationVariant::Normal : EDashAnimationVariant::Mirrored;
}

// [애니메이션-02] RunStop 진입 시 마지막 평가 커브로 정지 클립의 원본·미러를 선택한다.
void UPlayableAnimInstance::SelectRunStopClip()
{
	if (IsDashing())
		return;

	static const FName RunStopVariantCurveName(TEXT("RunStopVariant"));
	float RunStopVariantCurveValue = 0.0f;
	// RunStopVariantCurveValue: 1 or -1
	bNextRunStopIsMirrored = GetCurveValue(RunStopVariantCurveName, RunStopVariantCurveValue) && FMath::IsFinite(RunStopVariantCurveValue) && RunStopVariantCurveValue > 0.0f;
}
