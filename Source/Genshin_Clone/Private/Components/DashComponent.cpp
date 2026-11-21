// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/DashComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// [지상이동-02] 대시 규칙은 요청 시각을 비교하므로 컴포넌트 틱을 켜지 않는다.
UDashComponent::UDashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// [지상이동-02] 소유 캐릭터와 CharacterMovement의 필수 구성을 플레이 시작에 확인한다.
void UDashComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	checkf(OwnerCharacter, TEXT("DashComponent는 ACharacter 소유여야 합니다."));
	checkf(OwnerCharacter->GetCharacterMovement(), TEXT("DashComponent에 CharacterMovement가 필요합니다."));
}

// [지상이동-06] 시작 요청을 승인하고 실행 번호·재입력 잠금·연속 대시 이력을 갱신한다.
bool UDashComponent::TryStartDash(float RestartDelaySeconds)
{
	const double CurrentGameSeconds = GetWorld()->GetTimeSeconds();
	if (CurrentGameSeconds < DashUnlockTime || CurrentGameSeconds < DashCooldownEnd)
		return false;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
	if (!CharacterMovement->IsMovingOnGround())
		return false;

	DashRequestId = (DashRequestId == MAX_int32) ? 1 : DashRequestId + 1;
	DashUnlockTime = CurrentGameSeconds + RestartDelaySeconds;

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
