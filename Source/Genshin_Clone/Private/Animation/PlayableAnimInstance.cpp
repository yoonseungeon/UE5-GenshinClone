#include "Animation/PlayableAnimInstance.h"

#include "Characters/PlayableCharacter.h"
#include "Components/DashComponent.h"
#include "Components/PlayableMovementComponent.h"

// [지상이동-02] 루트 모션 정책과 최초 이동 계획을 설정한다.
UPlayableAnimInstance::UPlayableAnimInstance()
{
	RootMotionMode = ERootMotionMode::RootMotionFromEverything;
	UpdateMovementVariantPlan(false);
}

// 일반 출발은 RunCycle의 처음부터 시작한다.
void UPlayableAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	StartTimes.RunCycle = 0.0f;
}

// 게임 시작 시 컴포넌트에 남아 있는 과거 승인 번호를 흡수한다.
void UPlayableAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(TryGetPawnOwner());
	if (!PlayableCharacter)
		return;

	// [점프-09] 지상 상태머신의 조회 인덱스를 저장한다.
	GroundedMovementMachineIndex = GetStateMachineIndex(TEXT("GroundedMovement"));
	checkf(GroundedMovementMachineIndex != INDEX_NONE, TEXT("GroundedMovement 상태머신을 찾을 수 없습니다."));

	DashRequest.SnapshotId = PlayableCharacter->GetDashComponent()->GetDashRequestId();
	DashRequest.PendingId = DashRequest.SnapshotId;
	DashRequest.LastEnteredId = DashRequest.SnapshotId;
}

// [지상이동-05] SprintBS의 재입력 허용 시각으로 승인을 요청하고 성공한 요청 번호를 저장한다.
void UPlayableAnimInstance::RequestSprintStart()
{
	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(TryGetPawnOwner());
	if (!PlayableCharacter)
		return;

	UDashComponent* DashComponent = PlayableCharacter->GetDashComponent();
	if (!DashComponent->TryStartDash(TransitionTimes.SprintBSStopAllowedTime))
		return;

	DashRequest.PendingId = DashComponent->GetDashRequestId();
}

// [지상이동-07] 계속 바뀌는 입력과 지상 여부 및 승인 번호만 매 업데이트에서 갱신한다.
void UPlayableAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(TryGetPawnOwner());
	if (!PlayableCharacter)
		return;

	bHasMovementInput = !PlayableCharacter->GetMovementInputDirection().IsNearlyZero();
	// [점프-07] 현재 이동 모드에서 지상·루트 점프·중력 낙하 여부를 읽는다.
	const UPlayableMovementComponent* PlayableMovementComponent = static_cast<const UPlayableMovementComponent*>(PlayableCharacter->GetCharacterMovement());
	bIsMovingOnGround = PlayableMovementComponent->IsMovingOnGround();
	bIsRootMotionJumpActive = PlayableMovementComponent->IsRootMotionJumpActive();
	bIsFalling = PlayableMovementComponent->IsFalling();

	const UDashComponent* DashComponent = PlayableCharacter->GetDashComponent();
	bSprintInputHeld = DashComponent->IsSprintInputHeld();
	DashRequest.SnapshotId = DashComponent->GetDashRequestId();

	if (!bIsMovingOnGround)
		DashRequest.LastEnteredId = DashRequest.SnapshotId;
}

// [지상이동-08] 아직 반영하지 않은 새 승인 요청으로 자기 전환 여부를 판단한다.
bool UPlayableAnimInstance::HasNewDashRequest() const
{
	return bIsMovingOnGround && (DashRequest.SnapshotId == DashRequest.PendingId) && (DashRequest.SnapshotId != DashRequest.LastEnteredId);
}

// [지상이동-09] Run의 시간은 앞 상태에서 이미 준비했으므로 진입에서는 선택만 확정한다.
void UPlayableAnimInstance::SelectRunCycleClip()
{
	SelectedVariants.bRunCycleIsMirrored = VariantPlan.bRunCycleIsMirrored;
}

// RunStop 진입 순간의 커브로 정지 클립을 고르고 다음 이동 계획을 갱신한다.
void UPlayableAnimInstance::SelectRunStopClip()
{
	SelectedVariants.bRunStopIsMirrored = DoesCurveSelectMirroredClip(RunStopVariantCurveName);
	UpdateMovementVariantPlan(SelectedVariants.bRunStopIsMirrored);
	StartTimes.RunCycle = 0.0f;
}

// SprintStart 진입에서 다음 SprintBS 선택을 확정하고 진입한 요청 번호를 저장한다.
void UPlayableAnimInstance::SelectSprintBSClip()
{
	SelectedVariants.bSprintBSIsMirrored = VariantPlan.bSprintIsMirrored;
	DashRequest.LastEnteredId = DashRequest.SnapshotId;
}

// Sprint 진입에서 SprintBS와 같은 원본·미러의 SprintCycle을 고른다.
void UPlayableAnimInstance::SelectSprintCycleClip()
{
	SelectedVariants.bSprintCycleIsMirrored = SelectedVariants.bSprintBSIsMirrored;
}

// SprintToRun 진입에서 SprintBS 반대의 원본·미러를 고르고 이후 RunCycle 선택과 시작 위치를 준비한다.
void UPlayableAnimInstance::SelectSprintToRunClip()
{
	SelectedVariants.bSprintToRunIsMirrored = !SelectedVariants.bSprintBSIsMirrored;
	VariantPlan.bRunCycleIsMirrored = SelectedVariants.bSprintToRunIsMirrored;
	StartTimes.RunCycle = RunCycleAfterSprintToRunStartTime;
}

// SprintStop 진입 순간의 커브로 정지 클립을 고르고 다음 이동 계획을 갱신한다.
void UPlayableAnimInstance::SelectSprintStopClip()
{
	SelectedVariants.bSprintStopIsMirrored = DoesCurveSelectMirroredClip(RunStopVariantCurveName);
	UpdateMovementVariantPlan(SelectedVariants.bSprintStopIsMirrored);
	StartTimes.RunCycle = 0.0f;
}

// 최초 생성과 두 정지 상태 진입에서 전달받은 정지 선택으로 이동 계획을 만든다.
void UPlayableAnimInstance::UpdateMovementVariantPlan(bool bStopIsMirrored)
{
	VariantPlan.bRunCycleIsMirrored = !bStopIsMirrored;
	VariantPlan.bSprintIsMirrored = !bStopIsMirrored;
}

// 커브는 계속 변하지만 정지 클립은 정지 상태 진입 순간에 한 번 선택한다.
bool UPlayableAnimInstance::DoesCurveSelectMirroredClip(FName CurveName) const
{
	float CurveValue = 0.0f;
	const bool bHasCurveValue = GetCurveValue(CurveName, CurveValue);
	checkf(bHasCurveValue, TEXT("필수 애니메이션 커브 '%s'를 읽을 수 없습니다."), *CurveName.ToString());
	return CurveValue > 0.0f;
}

// [점프-09] 현재 지상 상태 이름에 따라 이번 Jump 애니메이션 종류를 고정한다.
void UPlayableAnimInstance::SelectJumpClip(bool bHasMovementInputAtJump)
{
	const FName GroundedStateName = GetCurrentStateName(GroundedMovementMachineIndex);

	if (GroundedStateName == TEXT("Standby"))
	{
		SelectedJumpClipType = EJumpClipType::Standing;
	}
	else if (GroundedStateName == TEXT("Run") || GroundedStateName == TEXT("SprintToRun"))
	{
		SelectedJumpClipType = EJumpClipType::Run;
	}
	else if (GroundedStateName == TEXT("SprintStart") || GroundedStateName == TEXT("Sprint"))
	{
		SelectedJumpClipType = EJumpClipType::Sprint;
	}
	else if (GroundedStateName == TEXT("RunStop") || GroundedStateName == TEXT("SprintStop"))
	{
		SelectedJumpClipType = bHasMovementInputAtJump ? EJumpClipType::Run : EJumpClipType::Standing;
	}
	else
	{
		checkf(false, TEXT("점프 종류가 정의되지 않은 지상 상태입니다: %s"), *GroundedStateName.ToString());
	}
}

// [점프-10] Landing 진입 시 현재 이동·Sprint 유지 입력으로 착지 종류를 한 번 선택한다.
void UPlayableAnimInstance::SelectLandingClip()
{
	if (!bHasMovementInput)
		SelectedLandingClipType = ELandingClipType::Standing;
	else if (bSprintInputHeld)
		SelectedLandingClipType = ELandingClipType::Sprint;
	else
		SelectedLandingClipType = ELandingClipType::Run;
}
