// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
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

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bShouldRun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bUseMirroredRunStop = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bUseMirroredRun = false;
};
