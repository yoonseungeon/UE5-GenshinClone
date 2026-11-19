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

	const FVector& GetMovementInputDirection() const { return MovementInputDirection; }

	class UDashComponent* GetDashComponent() const { return DashComponent.Get(); }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Cameras
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> Camera;

	// Input Action
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> MoveInputAction;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> LookInputAction;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> JumpInputAction;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<class UInputAction> DashInputAction;

private:
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);

	void RequestDash();
	void ClearMovementInputDirection();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UDashComponent> DashComponent;

	// 카메라 기준 이동 입력을 월드 방향으로 변환한 벡터다.
	FVector MovementInputDirection = FVector::ZeroVector;
};
