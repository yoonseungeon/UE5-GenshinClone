// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FinishRootMotionJump.generated.h"

UCLASS(meta = (DisplayName = "Finish Root Motion Jump"))
class GENSHIN_CLONE_API UAnimNotify_FinishRootMotionJump : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
