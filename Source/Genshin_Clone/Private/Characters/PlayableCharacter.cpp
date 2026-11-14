// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PlayableCharacter.h"
// Camera
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
// IA
#include "InputAction.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/RotationMatrix.h"

APlayableCharacter::APlayableCharacter()
{
	// [회전-01] Controller의 회전을 Actor에도 적용할지
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// [회전-01] CharacterMovement: Character에 선언되어 있음.
	// 캐릭터가 이동하려는 방향(가속 방향)으로 Actor를 돌린다.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// [카메라-01] Spring Arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));

	SpringArm->SetupAttachment(GetRootComponent());
	
	SpringArm->TargetArmLength = 400.f;
	// ControlRotation은 Controller가 가지고 있는 바라보는 방향
	SpringArm->bUsePawnControlRotation = true;

	// [카메라-02] Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// [입력-02] IA
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/Game/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/Game/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(TEXT("/Game/Game/Input/Actions/IA_Jump.IA_Jump"));
	static ConstructorHelpers::FObjectFinder<UInputAction> DashAsset(TEXT("/Game/Game/Input/Actions/IA_Dash.IA_Dash"));

	checkf(MoveAsset.Succeeded(), TEXT("IA_Move 로드 실패"));
	checkf(LookAsset.Succeeded(), TEXT("IA_Look 로드 실패"));
	checkf(JumpAsset.Succeeded(), TEXT("IA_Jump 로드 실패"));
	checkf(DashAsset.Succeeded(), TEXT("IA_Jump 로드 실패"));

	MoveInputAction = MoveAsset.Object;
	LookInputAction = LookAsset.Object;
	JumpInputAction = JumpAsset.Object;
	DashInputAction = DashAsset.Object;
}

// [입력-03] Controller가 Pawn을 처음 조종하거나 조종 대상을 변경할 때,
// DispatchRestart(true)를 거쳐 실행되는 APawn::PawnClientRestart() 내부에서 호출됨.
void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// PlayerInputComponent: Actor의 멤버 변수가 전달됨.
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	// IA에 함수를 매핑
	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &APlayableCharacter::Move);
	EnhancedInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &APlayableCharacter::Look);
	EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
}

// [입력-04] 이동 입력 처리
void APlayableCharacter::Move(const FInputActionValue& Value)
{
	// Controller: Pawn에 정의
	if (!Controller)
		return;

	const FVector2D Input = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Foward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Foward, Input.Y);
	AddMovementInput(Right, Input.X);
}

// [입력-05] 시점 입력 처리
void APlayableCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}
