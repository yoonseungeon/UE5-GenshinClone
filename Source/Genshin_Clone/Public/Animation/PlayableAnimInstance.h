// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "Components/DashComponent.h"

#include "PlayableAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GENSHIN_CLONE_API UPlayableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 애니메이션 계산은 게임 로직을 처리하는 게임 스레드와 별도의 작업 스레드에서 실행될 수 있다.
	// BlueprintThreadSafe: 애니메이션 작업 스레드에서 호출해도 안전하도록 작성했다고 엔진에 알림.
	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	void SelectRunStopVariant();

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	void SelectRunVariant();

	UFUNCTION(BlueprintCallable, Category = "Animation|Dash", meta = (BlueprintThreadSafe))
	void RememberDashStopVariant();

	// 복사한 대시 단계로 실행 여부를 제공한다.
	UFUNCTION(BlueprintPure, Category = "Animation|Dash", meta = (BlueprintThreadSafe))
	bool IsDashing() const { return DashState.Phase != EDashPhase::None; }
	
	EDashAnimationVariant GetDashStartVariant() const;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bShouldRun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bUseMirroredRunStop = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bUseMirroredRun = false;

	// [대시-10] AnimGraph에서 읽을 대시 상태와 Run 진입 데이터를 준비한다.
	// Run 상태 초기화에 사용할 초 단위 재생 시작 위치다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float RunStartPosition = 0.0f;

	// AnimGraph에서 읽을 대시 상태와 단계별 클립의 스냅샷이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Dash")
	FDashRuntimeState DashState;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Dash")
	TObjectPtr<class UAnimSequence> DashStartAnimation = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Dash")
	TObjectPtr<class UAnimSequence> DashStopAnimation = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Dash")
	TObjectPtr<class UAnimSequence> DashReturnAnimation = nullptr;

	// 정상 대시 복귀 변형과 Run 진입에서 마지막으로 소비한 실행 번호다.
	bool bDashReturnUsesMirroredRun = false;
	int32 LastConsumedDashActivationId = 0;
};
