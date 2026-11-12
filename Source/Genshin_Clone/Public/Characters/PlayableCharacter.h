// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBase.h"
#include "PlayableCharacter.generated.h"

/**
 * 
 */
UCLASS()
class GENSHIN_CLONE_API APlayableCharacter : public ACharacterBase
{
	GENERATED_BODY()
	
public:
	APlayableCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Cameras
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> Camera;

	// Input
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> MoveInputAction;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> LookInputAction;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> JumpInputAction;

private:
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);
};
