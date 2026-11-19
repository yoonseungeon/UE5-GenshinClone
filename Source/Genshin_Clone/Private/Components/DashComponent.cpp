// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/DashComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// [대시-02] 틱 설정과 원본·미러 애니메이션 세트의 기본값을 구성한다.
UDashComponent::UDashComponent()
{
	// 컴포넌트 틱을 허용한다.
	PrimaryComponentTick.bCanEverTick = true;
	// 대시 시작 전에는 틱을 비활성화한다.
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// 물리 시뮬레이션 전에 대시 단계를 갱신한다.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> SprintBSAsset(TEXT("/Game/Game/Characters/Player/Lumine/AM_DashStart.AM_DashStart"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> SprintStopAsset(TEXT("/Game/Game/Characters/Player/Lumine/AM_DashStop.AM_DashStop"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> SprintToRunAsset(TEXT("/Game/Game/Characters/Player/Lumine/AM_DashToRun.AM_DashToRun"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MirroredSprintBSAsset(TEXT("/Game/Game/Characters/Player/Lumine/AM_DashStart_Mirror.AM_DashStart_Mirror"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MirroredSprintStopAsset(TEXT("/Game/Game/Characters/Player/Lumine/AM_DashStop_Mirror.AM_DashStop_Mirror"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MirroredSprintToRunAsset(TEXT("/Game/Game/Characters/Player/Lumine/AM_DashToRun_Mirror.AM_DashToRun_Mirror"));

	checkf(SprintBSAsset.Succeeded(), TEXT("AM_DashStart 로드 실패"));
	checkf(SprintStopAsset.Succeeded(), TEXT("AM_DashStop 로드 실패"));
	checkf(SprintToRunAsset.Succeeded(), TEXT("AM_DashToRun 로드 실패"));
	checkf(MirroredSprintBSAsset.Succeeded(), TEXT("AM_DashStart_Mirror 로드 실패"));
	checkf(MirroredSprintStopAsset.Succeeded(), TEXT("AM_DashStop_Mirror 로드 실패"));
	checkf(MirroredSprintToRunAsset.Succeeded(), TEXT("AM_DashToRun_Mirror 로드 실패"));

	NormalDashAnimationSet.SprintBSMontage = SprintBSAsset.Object;
	NormalDashAnimationSet.SprintStopMontage = SprintStopAsset.Object;
	NormalDashAnimationSet.SprintToRunMontage = MirroredSprintToRunAsset.Object;
	NormalDashAnimationSet.SprintToRunStartRatio = 14.0f / 48.0f;
	NormalDashAnimationSet.bRunCycleIsMirrored = true;
	NormalDashAnimationSet.RunCyclePoseMatchTimeSeconds = 31.0f / 60.0f;

	MirroredDashAnimationSet.SprintBSMontage = MirroredSprintBSAsset.Object;
	MirroredDashAnimationSet.SprintStopMontage = MirroredSprintStopAsset.Object;
	MirroredDashAnimationSet.SprintToRunMontage = SprintToRunAsset.Object;
	MirroredDashAnimationSet.SprintToRunStartRatio = 14.0f / 48.0f;
	MirroredDashAnimationSet.bRunCycleIsMirrored = false;
	MirroredDashAnimationSet.RunCyclePoseMatchTimeSeconds = 31.0f / 60.0f;
}

// [대시-03] 소유 캐릭터의 기본 구성과 원본·미러 대시 몽타주를 플레이 시작에 확인한다.
void UDashComponent::BeginPlay()
{
	Super::BeginPlay();
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	checkf(OwnerCharacter, TEXT("DashComponent는 ACharacter 소유여야 합니다."));
	checkf(OwnerCharacter->GetCharacterMovement(), TEXT("DashComponent에 CharacterMovement가 필요합니다."));
	checkf(OwnerCharacter->GetMesh(), TEXT("DashComponent에 캐릭터 Mesh가 필요합니다."));
	checkf(OwnerCharacter->GetMesh()->GetAnimInstance(), TEXT("대시 몽타주를 재생할 애님 인스턴스가 Mesh에 연결되어 있어야 합니다."));
	checkf(NormalDashAnimationSet.SprintBSMontage && NormalDashAnimationSet.SprintStopMontage && NormalDashAnimationSet.SprintToRunMontage, TEXT("원본 대시 세트에 SprintBS·SprintStop·SprintToRun 몽타주가 필요합니다."));
	checkf(MirroredDashAnimationSet.SprintBSMontage && MirroredDashAnimationSet.SprintStopMontage && MirroredDashAnimationSet.SprintToRunMontage, TEXT("미러 대시 세트에 SprintBS·SprintStop·SprintToRun 몽타주가 필요합니다."));
}

// [대시-04] 현재 이동 입력 방향을 저장한다.
void UDashComponent::SetMovementInputDirection(const FVector& InputDirection)
{
	MovementInputDirection = InputDirection.GetSafeNormal2D();
}

// [대시-05] 재입력·쿨타임·지상 조건과 방향을 확인하고 대시 성공 후 연속 대시 이력을 저장한다.
bool UDashComponent::TryStartDash(EDashAnimationVariant DashVariant)
{
	const double CurrentGameSeconds = GetWorld()->GetTimeSeconds();
	if (CurrentGameSeconds < DashUnlockTime || CurrentGameSeconds < DashCooldownEnd)
		return false;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	if (!CharacterMovement->IsMovingOnGround())
		return false;

	FVector RequestedDashDirection = MovementInputDirection;
	if (RequestedDashDirection.IsNearlyZero())
		RequestedDashDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();

	if (!BeginDash(RequestedDashDirection, DashVariant))
		return false;

	const float SprintBSLengthSeconds = GetCurrentDashAnimationSet().SprintBSMontage->GetPlayLength();
	DashUnlockTime = CurrentGameSeconds + SprintBSLengthSeconds * SprintBSStopAllowedRatio;

	const bool bIsSecondDashInChain = bHasFirstDash && CurrentGameSeconds <= SecondDashDeadline;
	if (bIsSecondDashInChain)
	{
		bHasFirstDash = false;
		SecondDashDeadline = 0.0;
		DashCooldownEnd = CurrentGameSeconds + DashCooldown;
	}
	else
	{
		bHasFirstDash = true;
		SecondDashDeadline = CurrentGameSeconds + SecondDashTimeLimit;
	}
	return true;
}

// [대시-06] 변형·방향·실행 번호를 설정하고 Starting 단계에 진입한다.
bool UDashComponent::BeginDash(const FVector& RequestedDashDirection, EDashAnimationVariant DashVariant)
{
	const FVector NormalizedDashDirection = RequestedDashDirection.GetSafeNormal2D();
	if (NormalizedDashDirection.IsNearlyZero())
		return false;

	DashRuntimeState.AnimationVariant = DashVariant;
	DashRuntimeState.DashDirection = NormalizedDashDirection;
	DashRuntimeState.DashId = (DashRuntimeState.DashId == MAX_int32) ? 1 : DashRuntimeState.DashId + 1;
	DashRuntimeState.EndReason = EDashEndReason::None;
	return StartDashPhase(EDashPhase::Starting);
}

// [대시-07] 지정한 변형의 애니메이션 세트를 조회한다.
const FDashAnimationSet& UDashComponent::GetDashAnimationSet(EDashAnimationVariant DashVariant) const
{
	return DashVariant == EDashAnimationVariant::Mirrored ? MirroredDashAnimationSet : NormalDashAnimationSet;
}

// [대시-07] 현재 실행에서 고정한 변형의 애니메이션 세트를 조회한다.
const FDashAnimationSet& UDashComponent::GetCurrentDashAnimationSet() const
{
	return GetDashAnimationSet(DashRuntimeState.AnimationVariant);
}

// [대시-07] 지정한 변형의 세트에서 단계별 몽타주를 조회한다.
UAnimMontage* UDashComponent::GetDashPhaseMontage(EDashPhase DashPhase, EDashAnimationVariant DashVariant) const
{
	const FDashAnimationSet& DashAnimationSet = GetDashAnimationSet(DashVariant);
	switch (DashPhase)
	{
		case EDashPhase::Starting:
		{
			return DashAnimationSet.SprintBSMontage.Get();
		}
		case EDashPhase::Stopping:
		{
			return DashAnimationSet.SprintStopMontage.Get();
		}
		case EDashPhase::ReturningToRun:
		{
			return DashAnimationSet.SprintToRunMontage.Get();
		}
		default:
		{
			return nullptr;
		}
	}
}

// [대시-08] 실제 블렌드 길이와 경과 게임 시간으로 RunCycle 복귀 위치를 계산한다.
float UDashComponent::GetRunCycleStartTimeAfterDash() const
{
	if (DashRuntimeState.EndReason != EDashEndReason::ReturnedToRun)
		return 0.0f;

	const double SecondsSinceSprintToRunBlendOut = FMath::Max(GetWorld()->GetTimeSeconds() - SprintToRunBlendOutStartGameSeconds, 0.0);
	return FMath::Max(GetCurrentDashAnimationSet().RunCyclePoseMatchTimeSeconds - SprintToRunBlendOutSeconds + static_cast<float>(SecondsSinceSprintToRunBlendOut), 0.0f);
}

// [대시-09] 실제 몽타주 위치와 입력에 따라 단계를 전환한다.
void UDashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	if (!CharacterMovement->IsMovingOnGround())
	{
		CancelDash();
		return;
	}

	UAnimInstance* CharacterAnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
	FAnimMontageInstance* DashMontageInstance = CharacterAnimInstance->GetMontageInstanceForID(DashMontageInstanceId);
	if (!DashMontageInstance || !DashMontageInstance->Montage)
	{
		CancelDash();
		return;
	}

	DashRuntimeState.PhaseMontageElapsedSeconds = FMath::Max(DashMontageInstance->GetPosition() - DashRuntimeState.PhaseMontageStartTimeSeconds, 0.0f);
	const bool bHasMovementInput = !MovementInputDirection.IsNearlyZero();
	switch (DashRuntimeState.Phase)
	{
		case EDashPhase::Starting:
		{
			const float SprintBSLengthSeconds = DashMontageInstance->Montage->GetPlayLength();
			if (!bHasMovementInput && DashRuntimeState.PhaseMontageElapsedSeconds >= SprintBSLengthSeconds * SprintBSStopAllowedRatio)
			{
				StartDashPhase(EDashPhase::Stopping);
			}
			else if (bHasMovementInput && DashRuntimeState.PhaseMontageElapsedSeconds >= SprintBSLengthSeconds * SprintBSToSprintToRunRatio)
			{
				StartDashPhase(EDashPhase::ReturningToRun);
			}
			break;
		}
		case EDashPhase::Stopping:
		{
			if (bHasMovementInput)
				CancelDash();
			break;
		}
		case EDashPhase::ReturningToRun:
		{
			if (!bHasMovementInput)
			{
				StartDashPhase(EDashPhase::Stopping);
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

// [대시-10] 단계별 시작 위치를 설정하고 몽타주를 재생한다.
bool UDashComponent::StartDashPhase(EDashPhase DashPhase)
{
	float MontageStartRatio = 0.0f;
	if (DashPhase == EDashPhase::Stopping)
		MontageStartRatio = SprintStopStartRatio;
	else if (DashPhase == EDashPhase::ReturningToRun)
		MontageStartRatio = GetCurrentDashAnimationSet().SprintToRunStartRatio;

	UAnimMontage* DashMontage = GetDashPhaseMontage(DashPhase, DashRuntimeState.AnimationVariant);
	const float MontageStartTimeSeconds = DashMontage->GetPlayLength() * MontageStartRatio;
	DashRuntimeState.Phase = DashPhase;
	DashRuntimeState.PhaseMontageElapsedSeconds = 0.0f;
	DashRuntimeState.PhaseMontageStartTimeSeconds = MontageStartTimeSeconds;

	if (!PlayDashPhaseMontage(DashMontage, MontageStartTimeSeconds, DashRuntimeState.DashId))
	{
		CancelDash();
		return false;
	}

	if (DashPhase == EDashPhase::Stopping)
	{
		LastSprintStopDashId = DashRuntimeState.DashId;
		LastSprintStopVariant = DashRuntimeState.AnimationVariant;
	}
	SetComponentTickEnabled(true);
	return true;
}

// [대시-11] 취소 사유로 대시를 종료한다.
void UDashComponent::CancelDash()
{
	EndDash(EDashEndReason::Canceled);
}

// [대시-11] 종료 결과·변형·실행 번호는 보존하고 소유한 재생만 정리한다.
void UDashComponent::EndDash(EDashEndReason DashEndReason, bool bStopDashMontage)
{
	if (DashRuntimeState.Phase == EDashPhase::None)
		return;

	UAnimInstance* CharacterAnimInstance = Cast<ACharacter>(GetOwner())->GetMesh()->GetAnimInstance();
	const int32 OwnedDashMontageInstanceId = DashMontageInstanceId;
	DashMontageInstanceId = INDEX_NONE;

	DashRuntimeState.EndReason = DashEndReason;
	DashRuntimeState.Phase = EDashPhase::None;
	DashRuntimeState.PhaseMontageElapsedSeconds = 0.0f;
	DashRuntimeState.PhaseMontageStartTimeSeconds = 0.0f;
	DashRuntimeState.DashDirection = FVector::ZeroVector;
	SetComponentTickEnabled(false);

	if (bStopDashMontage && CharacterAnimInstance)
	{
		FAnimMontageInstance* DashMontageInstance = CharacterAnimInstance->GetMontageInstanceForID(OwnedDashMontageInstanceId);
		if (DashMontageInstance && DashMontageInstance->Montage)
			DashMontageInstance->Stop(FAlphaBlend(DashMontageInstance->Montage->GetDefaultBlendOutTime()));
	}
}

// [대시-12] 컴포넌트의 플레이가 종료될 때 진행 중인 대시도 정리한다.
void UDashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelDash();
	Super::EndPlay(EndPlayReason);
}

// [대시-13] 몽타주 재생과 종료 알림을 실제 인스턴스·대시 실행 번호로 연결한다.
bool UDashComponent::PlayDashPhaseMontage(UAnimMontage* DashMontage, float MontageStartTimeSeconds, int32 DashId)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UAnimInstance* CharacterAnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();

	const int32 PreviousDashMontageInstanceId = DashMontageInstanceId;

	// 이전 재생의 중단 알림이 새 단계를 취소하지 못하게 먼저 무효화한다.
	DashMontageInstanceId = INDEX_NONE;

	const float MontageLengthSeconds = CharacterAnimInstance->Montage_Play(DashMontage, 1.0f, EMontagePlayReturnType::MontageLength, MontageStartTimeSeconds);
	// 재생에 성공하면 현재 재생 위치와 블렌드 상태를 담은 몽타주 실행 객체를 가져온다.
	FAnimMontageInstance* DashMontageInstance = MontageLengthSeconds > 0.0f ? CharacterAnimInstance->GetActiveInstanceForMontage(DashMontage) : nullptr;
	if (!DashMontageInstance)
	{
		// 실패 시 호출자가 기존 재생도 정리할 수 있도록 복구한다.
		DashMontageInstanceId = PreviousDashMontageInstanceId;
		return false;
	}

	DashMontageInstanceId = DashMontageInstance->GetInstanceID();
	// 현재 단계에 필요한 알림만 등록한다.
	switch (DashRuntimeState.Phase)
	{
		case EDashPhase::Stopping:
		{
			DashMontageInstance->OnMontageEnded.BindUObject(this, &UDashComponent::OnSprintStopMontageEnded, DashMontageInstanceId, DashId);
			break;
		}
		case EDashPhase::ReturningToRun:
		{
			DashMontageInstance->OnMontageBlendingOutStarted.BindUObject(this, &UDashComponent::OnSprintToRunMontageBlendOutStarted, DashMontageInstanceId, DashId);
			break;
		}
		default:
		{
			break;
		}
	}
	return true;
}

// [대시-14] SprintStop의 재생과 블렌드가 모두 끝나면 정지 완료를 기록한다.
void UDashComponent::OnSprintStopMontageEnded(UAnimMontage* DashMontage, bool bInterrupted, int32 SourceMontageInstanceId, int32 SourceDashId)
{
	if (DashMontageInstanceId != SourceMontageInstanceId || DashRuntimeState.DashId != SourceDashId || DashRuntimeState.Phase == EDashPhase::None)
		return;

	EndDash(bInterrupted ? EDashEndReason::Canceled : EDashEndReason::Stopped, false);
}

// [대시-15] SprintToRun의 정상 블렌드 아웃 시작에 RunCycle 복귀 시간을 저장한다.
void UDashComponent::OnSprintToRunMontageBlendOutStarted(UAnimMontage* DashMontage, bool bInterrupted, int32 SourceMontageInstanceId, int32 SourceDashId)
{
	if (DashMontageInstanceId != SourceMontageInstanceId || DashRuntimeState.DashId != SourceDashId || DashRuntimeState.Phase == EDashPhase::None)
		return;

	if (bInterrupted)
	{
		EndDash(EDashEndReason::Canceled, false);
		return;
	}

	UAnimInstance* CharacterAnimInstance = Cast<ACharacter>(GetOwner())->GetMesh()->GetAnimInstance();
	FAnimMontageInstance* DashMontageInstance = CharacterAnimInstance->GetMontageInstanceForID(SourceMontageInstanceId);
	SprintToRunBlendOutSeconds = DashMontageInstance ? DashMontageInstance->GetBlendTime() : 0.0f;
	SprintToRunBlendOutStartGameSeconds = GetWorld()->GetTimeSeconds();
	EndDash(EDashEndReason::ReturnedToRun, false);
}
