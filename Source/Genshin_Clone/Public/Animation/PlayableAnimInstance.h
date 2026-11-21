#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/PlayableLocomotionTypes.h"
#include "PlayableAnimInstance.generated.h"

UCLASS()
class GENSHIN_CLONE_API UPlayableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPlayableAnimInstance();

	// [지상이동-05] SprintBS의 재입력 허용 시각으로 대시 시작 승인을 요청한다.
	void RequestSprintStart();

	// [지상이동-08] 아직 SprintStart에 반영하지 않은 승인 요청을 확인한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	bool HasNewDashRequest() const;

	// [지상이동-09] 각 상태 진입에서 해당 상태의 원본과 미러 선택만 확정한다.
	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	void SelectRunCycleClip();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	void SelectRunStopClip();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	void SelectSprintBSClip();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	void SelectSprintCycleClip();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	void SelectSprintToRunClip();

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion", meta = (BlueprintThreadSafe))
	void SelectSprintStopClip();

protected:
	// [지상이동-02] 그래프 초기화 전에 RunCycle의 일반 출발 위치를 초기화한다.
	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	// 매 업데이트에서 갱신하는 입력과 지상 여부다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bHasMovementInput = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bIsMovingOnGround = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintInputHeld = false;

	// 각 상태의 Bool 블렌드에서 해당 필드만 읽는다. 참이면 미러 클립이다.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion")
	FLocomotionVariants SelectedVariants;

	// 각 상태의 원본과 미러 Player가 같은 시작 위치를 읽는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	FLocomotionStartTimes StartTimes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing", meta = (Units = "s"))
	float RunCycleAfterSprintToRunStartTime = 31.0f / 60.0f - 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing")
	FLocomotionTransitionTimes TransitionTimes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing", meta = (Units = "s"))
	float TransitionBlendTime = 0.1f;

private:
	void UpdateMovementVariantPlan(bool bStopIsMirrored);
	bool DoesCurveSelectMirroredClip(FName CurveName) const;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Curves")
	FName RunStopVariantCurveName = TEXT("RunStopVariant");

	// 다음 RunCycle과 SprintBS 진입에서 고를 원본·미러 선택이다.
	FMovementVariantPlan VariantPlan;

	FDashRequestState DashRequest;
};
