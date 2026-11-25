// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PlayableMovementComponent.generated.h"

enum class EPlayableCustomMovementMode : uint8
{
	RootMotionJump = 1
};

UCLASS()
class GENSHIN_CLONE_API UPlayableMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	bool IsRootMotionJumpActive() const;
	void FinishRootMotionJump();

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

private:
	void PhysRootMotionJump(float DeltaTime, int32 Iterations);
};
