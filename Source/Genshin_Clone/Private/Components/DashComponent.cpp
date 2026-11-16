// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/DashComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Animation/AnimSequence.h"

// [대시-02] 틱 설정과 원본·미러 애니메이션 세트의 기본값을 구성한다.
UDashComponent::UDashComponent()
{
	// 틱 사용을 허용한다.
	PrimaryComponentTick.bCanEverTick = true;
	// 대시 시작 전에는 틱을 비활성화한다.
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// 물리 시뮬레이션 전에 대시 단계를 갱신한다.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// 원본 세트의 기본 클립을 불러온다.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> StartAsset(TEXT("/Game/ExternalAsset/Characters/Common/Animations/Common_SprintBS.Common_SprintBS"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> StopAsset(TEXT("/Game/ExternalAsset/Characters/Common/Animations/Common_SprintStopL.Common_SprintStopL"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ReturnAsset(TEXT("/Game/ExternalAsset/Characters/Common/Animations/Common_SprintToRun.Common_SprintToRun"));

	// 미러 세트의 기본 클립을 불러온다.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> MirroredStartAsset(TEXT("/Game/ExternalAsset/Characters/Common/Animations/Common_SprintBS+Mirror.Common_SprintBS+Mirror"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> MirroredStopAsset(TEXT("/Game/ExternalAsset/Characters/Common/Animations/Common_SprintStopL+Mirror.Common_SprintStopL+Mirror"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> MirroredReturnAsset(TEXT("/Game/ExternalAsset/Characters/Common/Animations/Common_SprintToRun+Mirror.Common_SprintToRun+Mirror"));

	checkf(StartAsset.Succeeded(), TEXT("Common_SprintBS 로드 실패"));
	checkf(StopAsset.Succeeded(), TEXT("Common_SprintStopL 로드 실패"));
	checkf(ReturnAsset.Succeeded(), TEXT("Common_SprintToRun 로드 실패"));

	checkf(MirroredStartAsset.Succeeded(), TEXT("Common_SprintBS+Mirror 로드 실패"));
	checkf(MirroredStopAsset.Succeeded(), TEXT("Common_SprintStopL+Mirror 로드 실패"));
	checkf(MirroredReturnAsset.Succeeded(), TEXT("Common_SprintToRun+Mirror 로드 실패"));

	NormalAnimationSet.StartAnimation = StartAsset.Object;
	NormalAnimationSet.StopAnimation = StopAsset.Object;
	NormalAnimationSet.ReturnAnimation = MirroredReturnAsset.Object;
	NormalAnimationSet.ReturnAnimationStartRatio = 14.0f / 48.0f;
	NormalAnimationSet.bUseMirroredRun = true;
	NormalAnimationSet.RunPoseMatchTime = 31.0f / 60.0f;

	MirroredAnimationSet.StartAnimation = MirroredStartAsset.Object;
	MirroredAnimationSet.StopAnimation = MirroredStopAsset.Object;
	MirroredAnimationSet.ReturnAnimation = ReturnAsset.Object;
	MirroredAnimationSet.ReturnAnimationStartRatio = 14.0f / 48.0f;
	MirroredAnimationSet.bUseMirroredRun = false;
	MirroredAnimationSet.RunPoseMatchTime = 31.0f / 60.0f;
}

// [대시-03] 현재 사용자 이동 의도를 저장한다.
void UDashComponent::UpdateMovementIntent(const FVector& Intent)
{
	CurrentMovementIntent = Intent.GetSafeNormal2D();
}

// [대시-04] 기본 시작 조건을 확인하고 대시 방향을 선택한다.
bool UDashComponent::TryStartDash(EDashAnimationVariant Variant)
{
	// 이번 시작에서 선택할 세트의 필수 클립을 확인한다.
	const FDashAnimationSet& AnimationSet = GetAnimationSet(Variant);
	if (!IsValid(AnimationSet.StartAnimation) || !IsValid(AnimationSet.StopAnimation) || !IsValid(AnimationSet.ReturnAnimation))
		return false;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character) || RuntimeState.Phase != EDashPhase::None)
		return false;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!IsValid(Movement) || !Movement->IsMovingOnGround())
		return false;

	FVector Direction = CurrentMovementIntent;
	if (Direction.IsNearlyZero())
		Direction = Character->GetActorForwardVector().GetSafeNormal2D();

	return BeginDash(Direction, Variant);
}

// [대시-05] 선택 세트·방향·실행 번호를 고정하고 대시 갱신을 시작한다.
bool UDashComponent::BeginDash(const FVector& Direction, EDashAnimationVariant Variant)
{
	const FVector DashDirection = Direction.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
		return false;

	RuntimeState.Variant = Variant;

	RuntimeState.Phase = EDashPhase::Starting;
	RuntimeState.EndReason = EDashEndReason::None;
	RuntimeState.PhaseElapsedTime = 0.0f;
	RuntimeState.AnimationStartTime = 0.0f;
	RuntimeState.Direction = DashDirection;
	RuntimeState.ActivationId = (RuntimeState.ActivationId == MAX_int32) ? 1 : RuntimeState.ActivationId + 1;
	// 대시 단계 갱신을 시작한다.
	SetComponentTickEnabled(true);
	return true;
}

// [대시-06] 지정한 변형의 애니메이션 세트를 조회한다.
const FDashAnimationSet& UDashComponent::GetAnimationSet(EDashAnimationVariant Variant) const
{
	return Variant == EDashAnimationVariant::Mirrored ? MirroredAnimationSet : NormalAnimationSet;
}

// 현재 실행에서 고정한 변형의 애니메이션 세트를 조회한다.
const FDashAnimationSet& UDashComponent::GetActiveAnimationSet() const
{
	return GetAnimationSet(RuntimeState.Variant);
}

// 현재 실행에서 고정한 세트의 단계별 클립을 조회한다.
UAnimSequence* UDashComponent::GetAnimationForPhase(EDashPhase Phase) const
{
	const FDashAnimationSet& AnimationSet = GetActiveAnimationSet();
	switch (Phase)
	{
		case EDashPhase::Starting:
		{
			return AnimationSet.StartAnimation.Get();
		}
		case EDashPhase::Stopping:
		{
			return AnimationSet.StopAnimation.Get();
		}
		case EDashPhase::ReturningToRun:
		{
			return AnimationSet.ReturnAnimation.Get();
		}
		default:
		{
			return nullptr;
		}
	}
}

// [대시-07] 입력과 시간에 따라 단계를 전환하고 대시를 종료한다.
void UDashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (RuntimeState.Phase == EDashPhase::None)
		return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
	if (!IsValid(Movement) || !Movement->IsMovingOnGround())
	{
		CancelDash();
		return;
	}

	RuntimeState.PhaseElapsedTime += DeltaTime;
	const bool bHasMovementInput = !CurrentMovementIntent.IsNearlyZero();
	UAnimSequence* Animation = GetAnimationForPhase(RuntimeState.Phase);
	if (!IsValid(Animation))
	{
		CancelDash();
		return;
	}

	// 시작 위치 이후의 남은 재생 시간을 사용한다.
	const float Duration = FMath::Max(Animation->GetPlayLength() - RuntimeState.AnimationStartTime, 0.0f);
	switch (RuntimeState.Phase)
	{
		case EDashPhase::Starting:
			if (!bHasMovementInput && RuntimeState.PhaseElapsedTime >= Duration * StopAllowedStartRatio)
				EnterPhase(EDashPhase::Stopping);
			else if (bHasMovementInput && RuntimeState.PhaseElapsedTime >= Duration * StartToReturnRatio)
				EnterPhase(EDashPhase::ReturningToRun);
			break;

		// 정지 완료와 취소를 구분하고 정상 복귀 블렌드를 앞당긴다.
		case EDashPhase::Stopping:
			if (bHasMovementInput)
				EndDash(EDashEndReason::Canceled);
			else if (RuntimeState.PhaseElapsedTime >= Duration)
				EndDash(EDashEndReason::Stopped);
			break;

		case EDashPhase::ReturningToRun:
			if (!bHasMovementInput)
				EnterPhase(EDashPhase::Stopping);
			else if (RuntimeState.PhaseElapsedTime >= FMath::Max(Duration - ReturnToRunBlendTime, 0.0f))
				EndDash(EDashEndReason::ReturnedToRun);
			break;

		default:
			break;
	}
}

// [대시-08] 단계 시간을 초기화하고 중간 재생 시작 위치를 초로 계산한다.
void UDashComponent::EnterPhase(EDashPhase Phase)
{
	if (Phase == EDashPhase::None)
	{
		CancelDash();
		return;
	}

	RuntimeState.Phase = Phase;
	RuntimeState.PhaseElapsedTime = 0.0f;
	RuntimeState.AnimationStartTime = 0.0f;

	const FDashAnimationSet& AnimationSet = GetActiveAnimationSet();
	if (Phase == EDashPhase::Stopping)
		RuntimeState.AnimationStartTime = AnimationSet.StopAnimation->GetPlayLength() * StopAnimationStartRatio;
	else if (Phase == EDashPhase::ReturningToRun)
		RuntimeState.AnimationStartTime = AnimationSet.ReturnAnimation->GetPlayLength() * AnimationSet.ReturnAnimationStartRatio;
}

// [대시-09] 취소 사유로 대시를 종료한다.
void UDashComponent::CancelDash()
{
	EndDash(EDashEndReason::Canceled);
}

// 종료 결과를 기록하고 변형·실행 번호를 보존한 채 실행 상태를 정리한다.
void UDashComponent::EndDash(EDashEndReason Reason)
{
	if (RuntimeState.Phase == EDashPhase::None)
		return;

	RuntimeState.EndReason = Reason;
	RuntimeState.Phase = EDashPhase::None;
	RuntimeState.PhaseElapsedTime = 0.0f;
	RuntimeState.AnimationStartTime = 0.0f;
	RuntimeState.Direction = FVector::ZeroVector;
	SetComponentTickEnabled(false);
}
