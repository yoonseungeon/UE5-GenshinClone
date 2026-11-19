// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "Components/DashComponent.h"
#include "Animation/AnimNodeReference.h"

#include "PlayableAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GENSHIN_CLONE_API UPlayableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 애니메이션 계산은 게임 로직이 실행되는 게임 스레드와 별도의 작업 스레드에서 실행될 수 있다.
	// BlueprintThreadSafe: 애니메이션 작업 스레드에서 호출해도 안전하도록 작성했다고 엔진에 알림.
	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	void SelectRunStopClip();

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	void SelectRunCycleClip();

	// [대시-17] 선택된 RunCycle 노드의 재생 시간을 대시 복귀 시간으로 한 번 맞춘다.
	UFUNCTION(BlueprintCallable, Category = "Animation|Dash", meta = (BlueprintThreadSafe))
	void SetRunCycleTimeAfterDash(const FAnimNodeReference& RunCycleNode, bool bRunCycleNodeIsMirrored);

	// 복사한 대시 단계로 실행 여부를 제공한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Dash", meta = (BlueprintThreadSafe))
	bool IsDashing() const { return DashStateSnapshot.Phase != EDashPhase::None; }
	
	// [대시-18] RunStopVariant 커브 또는 마지막 정지 클립으로 시작할 대시 변형을 고른다.
	EDashAnimationVariant GetDashStartVariant() const;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bHasMovementInput = false;

	// 다음 RunStop 진입에 재생할 클립의 미러 여부이며 재생 중에도 유지된다. 마지막 RunStop/SprintStop 기억도 공유한다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bNextRunStopIsMirrored = false;

	// 다음 RunCycle 진입에 재생할 클립의 미러 여부이며 재생 중에도 유지된다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bNextRunCycleIsMirrored = false;

	// [대시-16] AnimGraph에서 읽을 대시 상태와 RunCycle 재생 시간을 준비한다.
	// RunCycle 초기화 또는 대시 복귀 시 맞출 초 단위 재생 시간이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float RunCycleStartTimeSeconds = 0.0f;

	// AnimGraph에서 읽을 대시 상태의 스냅샷이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Dash")
	FDashRuntimeState DashStateSnapshot;

private:
	// 선택된 RunCycle 노드의 재생 시간을 대시 복귀 시간으로 아직 맞추지 않았다.
	bool bRunCycleNeedsTimeSyncAfterDash = false;

	// RunCycle 시간 맞춤을 완료했거나 이동 입력이 없어 건너뛴 마지막 대시 번호다.
	int32 LastRunCycleReturnDashId = 0;
	// 공유 정지 기억에 마지막으로 저장한 SprintStop의 대시 번호다.
	int32 LastRecordedSprintStopDashId = 0;
};
