// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DashComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GENSHIN_CLONE_API UDashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// [지상이동-02] 틱 없이 대시 시작 규칙과 Sprint 유지 입력을 보관한다.
	UDashComponent();

	// [지상이동-06] 지상·재입력·쿨타임 조건을 통과한 대시 요청을 승인한다.
	bool TryStartDash(float RestartDelaySeconds);

	// [지상이동-04] 우클릭 유지 입력은 시작 승인 여부와 독립적으로 저장한다.
	void SetSprintInputHeld(bool bHeld) { bSprintInputHeld = bHeld; }
	bool IsSprintInputHeld() const { return bSprintInputHeld; }
	int32 GetDashRequestId() const { return DashRequestId; }

protected:
	// [지상이동-02] 소유 캐릭터와 CharacterMovement의 필수 구성을 한 번 확인한다.
	virtual void BeginPlay() override;

private:
	// 첫 대시 시작 후 두 번째 대시를 인정하는 시간이다.
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (Units = "s"))
	float SecondDashTimeLimit = 0.8f;

	// 두 번째 대시 승인부터 적용할 쿨타임이다.
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (Units = "s"))
	float DashCooldown = 1.5f;

	// 승인할 때 증가하며 애님 인스턴스가 새 시작 요청을 구분한다.
	int32 DashRequestId = 0;

	// 우클릭 유지 여부이며 대시 시작 승인 여부와 독립적이다.
	bool bSprintInputHeld = false;

	// 연속 대시의 첫 번째 승인 기록이 있다.
	bool bHasFirstDash = false;

	// 두 번째 대시를 인정하는 마지막 게임 시각이다.
	double SecondDashDeadline = 0.0;

	// SprintBS 재시작을 허용하는 게임 시각이다.
	double DashUnlockTime = 0.0;

	// 연속 대시 쿨타임이 끝나는 게임 시각이다.
	double DashCooldownEnd = 0.0;
};
