// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PlayableMovementComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"

// [점프-02] 지상 점프를 승인하고 애니메이션의 XYZ 루트 이동 구간으로 진입한다.
bool UPlayableMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if (!CharacterOwner || !CharacterOwner->CanJump() || !IsMovingOnGround())
		return false;
	if (bConstrainToPlane && FMath::IsNearlyEqual(FMath::Abs(GetGravitySpaceZ(PlaneConstraintNormal)), 1.f))
		return false;

	// 점프 직전의 달리기 속도를 지움.
	Velocity = FVector::ZeroVector;
	// MOVE_Custom을 보고 PhysCustom()을 호출
	SetMovementMode(MOVE_Custom, static_cast<uint8>(EPlayableCustomMovementMode::RootMotionJump));
	return true;
}

// [점프-03] 현재 이동 모드가 애니메이션 루트 점프인지 반환한다.
bool UPlayableMovementComponent::IsRootMotionJumpActive() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(EPlayableCustomMovementMode::RootMotionJump);
}

// [점프-04] 마지막 이동 속도를 유지한 채 중력 낙하로 전환한다.
void UPlayableMovementComponent::FinishRootMotionJump()
{
	// Velocity를 변경하는 코드가 없으므로, 호출 직전의 속도는 유지
	// ProcessLanded() 안에서 지상 이동 모드로 바꾸는 부분이 Falling일 때만 실행되기 때문에 MOVE_Falling로
	if (IsRootMotionJumpActive())
		SetMovementMode(MOVE_Falling);
}

// [점프-05] 엔진의 Custom 이동 업데이트를 루트 점프 이동 함수로 연결한다.
void UPlayableMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (IsRootMotionJumpActive())
	{
		PhysRootMotionJump(DeltaTime, Iterations);
		return;
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

// [점프-06] 중력 없이 루트 속도로 캡슐을 이동하고 벽 충돌과 하강 착지를 판단한다.
void UPlayableMovementComponent::PhysRootMotionJump(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !HasAnimRootMotion())
		return;

	// 가산형 Root Motion Source가 적용됐다면, 루트 모션 속도를 더하기 전 값으로 복원한다.
	RestorePreAdditiveRootMotionVelocity();

	// Falling 모드 였으면, 루트 모션 z 속도가 아니라 중력에 의한 속도가 적용
	ApplyRootMotionToVelocity(DeltaTime);

	// 물리 처리를 몇 번 반복했는지를 관리하는 변수야.
	// 언리얼은 한 프레임에 하나의 이동 모드만 처리하는 구조가 아님
	// 한 프레임 도중에도 이동 모드가 변경될 수 있고, 지나치게 반복되는거 방지
	++Iterations;

	// 이번 이동 중 위치가 불연속적으로 보정됐는가
	bJustTeleported = false;

	const FVector MovementDelta = Velocity * DeltaTime;
	FHitResult Hit;
	// UpdatedComponent: MovementComponent가 실제로 위치와 회전을 바꾸는 대상 컴포넌트
	// UCharacterMovementComponent의 이동 대상인 UpdatedComponent는 UCapsuleComponent여야 함.
	// true: 이동 경로 따라 충돌 검사
	SafeMoveUpdatedComponent(MovementDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
	// 충돌로 캐릭터 삭제, 모드 전환 검사
	if (!HasValidData() || !IsRootMotionJumpActive() || !Hit.IsValidBlockingHit())
		return;

	const bool bIsDescending = GetGravitySpaceZ(Velocity) <= 0.f;
	if (bIsDescending && IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
	{
		FinishRootMotionJump();
		// 착지 경로에서 충돌 힘도 전달하므로 HandleImpact보다 먼저
		ProcessLanded(Hit, DeltaTime * (1.f - Hit.Time), Iterations);
		return;
	}

	// 착지 아니면 충돌 처리
	HandleImpact(Hit, DeltaTime, MovementDelta);
	if (!HasValidData() || !IsRootMotionJumpActive())
		return;

	// 이 함수 안에서는 최대 두 번의 슬라이딩 이동을 시도
	const float SlideTime = DeltaTime * (1.f - Hit.Time);
	const float SlideTimeFraction = SlideAlongSurface(MovementDelta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	if (!HasValidData() || !IsRootMotionJumpActive())
		return;

	if (bIsDescending && Hit.IsValidBlockingHit() && IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
	{
		FinishRootMotionJump();
		// 슬라이딩 이후 남은 시간으로 착지 후 이동 계산을 이어간다.
		ProcessLanded(Hit, SlideTime * (1.f - SlideTimeFraction), Iterations);
	}
}
