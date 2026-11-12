// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/MainPlayerController.h"
#include "EnhancedInputSubSystems.h"
#include "InputMappingContext.h"

// [입력-01] IMC 에셋 로드
AMainPlayerController::AMainPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCAsseet(TEXT("/Game/Game/Input/IMC_Player.IMC_Player"));

	checkf(IMCAsseet.Succeeded(), TEXT("MainPlayerController.cpp: IMC를 찾지 못했습니다."));
	if (IMCAsseet.Succeeded())
	{
		PlayerIMC = IMCAsseet.Object;
	}
}

void AMainPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// [입력-01] UPlayer를 캐스팅해서 ULocalPlayer를 가져옴.
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	// LocalPlayer 내부에 FObjectSubsystemCollection 변수기 있음.
	// GetSubsystem: FObjectSubsystemCollection에서 원하는 Subsystem을 찾음.
	UEnhancedInputLocalPlayerSubsystem* SubSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (SubSystem)
	{
		// 사용할 IMC 등록
		// 내부에서 ULocalPlayer → APlayerController → PlayerInput에 IMC 등록
		// e.g.)
		// PlayerInput이 IMC를 보고 W 키를 IA_Move로 해석함.
		// PlayerInput이 PlayerInputComponent의 등록 내용을 보고 Move()를 실행함.
		// Controller가 PlayerInputComponent를 PlayerInput에게 전달하고, PlayerInput이 받아서 처리하는 관계
		SubSystem->AddMappingContext(PlayerIMC, 0);
	}
}
