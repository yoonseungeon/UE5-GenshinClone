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

#include "Components/DashComponent.h"

#include "Animation/PlayableAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

#include "Components/PlayableMovementComponent.h"

APlayableCharacter::APlayableCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayableMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// [회전-01] Controller의 회전을 Actor에도 적용할지
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// [회전-01] CharacterMovement: Character에 선언되어 있음.
	// 캐릭터가 이동하려는 방향(가속 방향)으로 Actor를 돌린다.
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// 애니메이션 루트 모션 중에도 이동 입력에 따른 회전을 허용한다.
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;
	// 회전 속도
	GetCharacterMovement()->RotationRate = FRotator(0.f, 900.f, 0.f);

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

	// [지상이동-02] 대시 시작 승인과 Sprint 유지 입력을 보관할 컴포넌트를 만든다.
	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));

	// [입력-02] IA
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/Game/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/Game/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(TEXT("/Game/Game/Input/Actions/IA_Jump.IA_Jump"));
	static ConstructorHelpers::FObjectFinder<UInputAction> DashAsset(TEXT("/Game/Game/Input/Actions/IA_Dash.IA_Dash"));

	checkf(MoveAsset.Succeeded(), TEXT("IA_Move 로드 실패"));
	checkf(LookAsset.Succeeded(), TEXT("IA_Look 로드 실패"));
	checkf(JumpAsset.Succeeded(), TEXT("IA_Jump 로드 실패"));
	checkf(DashAsset.Succeeded(), TEXT("IA_Dash 로드 실패"));

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

	// [지상이동-03] 우클릭 시작은 대시 요청으로, 해제·취소는 Sprint 유지 입력 해제로 연결한다.
	EnhancedInputComponent->BindAction(DashInputAction, ETriggerEvent::Started, this, &APlayableCharacter::RequestDash);
	EnhancedInputComponent->BindAction(DashInputAction, ETriggerEvent::Completed, this, &APlayableCharacter::ReleaseSprintInput);
	EnhancedInputComponent->BindAction(DashInputAction, ETriggerEvent::Canceled, this, &APlayableCharacter::ReleaseSprintInput);

	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &APlayableCharacter::ClearMovementInputDirection);
	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &APlayableCharacter::ClearMovementInputDirection);
}

// [점프-09] 엔진이 DoJump()의 반환값을 보고 점프가 성공했다고 판단하면 호출
void APlayableCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	UPlayableAnimInstance* PlayableAnimInstance = CastChecked<UPlayableAnimInstance>(GetMesh()->GetAnimInstance());
	PlayableAnimInstance->SelectJumpClip(!MovementInputDirection.IsNearlyZero());
}

// [입력-04] 카메라 기준 입력을 월드 방향으로 바꾸고 캐릭터 이동에 전달한다.
void APlayableCharacter::Move(const FInputActionValue& Value)
{
	// Controller: Pawn에 정의
	if (!Controller)
		return;

	const FVector2D MovementInput = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	MovementInputDirection = ForwardDirection * MovementInput.Y + RightDirection * MovementInput.X;

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

// [입력-05] 시점 입력만큼 카메라의 좌우·상하 각도를 바꾼다.
void APlayableCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}

// [지상이동-04] 우클릭 유지를 먼저 저장하고 현재 애님 인스턴스에 SprintStart 시작을 요청한다.
void APlayableCharacter::RequestDash()
{
	// 우클릭이 눌려 있다는 사실을 저장
	DashComponent->SetSprintInputHeld(true);
	UPlayableAnimInstance* PlayableAnimInstance = Cast<UPlayableAnimInstance>(GetMesh()->GetAnimInstance());
	if (!PlayableAnimInstance)
		return;

	// 애님 인스턴스에 이번 SprintBS를 선택하고 대시 승인을 요청하라고 전달
	PlayableAnimInstance->RequestSprintStart();
}

// [지상이동-04] 우클릭 해제·취소 시 Sprint 유지 입력을 끈다.
void APlayableCharacter::ReleaseSprintInput()
{
	DashComponent->SetSprintInputHeld(false);
}

// [지상이동-03] 이동키를 놓으면 캐릭터의 이동 입력 방향을 0으로 만든다.
void APlayableCharacter::ClearMovementInputDirection()
{
	MovementInputDirection = FVector::ZeroVector;
}
