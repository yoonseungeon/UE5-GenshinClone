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

APlayableCharacter::APlayableCharacter()
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

	// [대시-02] 대시 컴포넌트를 만들고 실제 이동 처리보다 먼저 갱신되도록 연결한다.
	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));
	GetCharacterMovement()->AddTickPrerequisiteComponent(DashComponent);

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

	// [대시-04] 우클릭은 대시를 요청하고, 이동키 해제는 입력 방향을 0으로 만든다.
	EnhancedInputComponent->BindAction(DashInputAction, ETriggerEvent::Started, this, &APlayableCharacter::RequestDash);

	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &APlayableCharacter::ClearMovementInputDirection);
	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &APlayableCharacter::ClearMovementInputDirection);
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
	DashComponent->SetMovementInputDirection(MovementInputDirection);

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

// [대시-04] 애님 인스턴스가 추천한 변형을 대시 시작 요청으로 전달한다.
void APlayableCharacter::RequestDash()
{
	UPlayableAnimInstance* PlayableAnimInstance = CastChecked<UPlayableAnimInstance>(GetMesh()->GetAnimInstance());
	DashComponent->TryStartDash(PlayableAnimInstance->GetDashStartVariant());
}

// [대시-04] 이동키를 놓으면 캐릭터와 대시에 저장된 입력 방향을 0으로 만든다.
void APlayableCharacter::ClearMovementInputDirection()
{
	MovementInputDirection = FVector::ZeroVector;
	DashComponent->SetMovementInputDirection(MovementInputDirection);
}
