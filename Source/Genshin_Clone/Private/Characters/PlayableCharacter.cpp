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

	// [대시-03] 우클릭은 대시를 요청하고, 이동키 해제는 이동 의도를 초기화한다.
	EnhancedInputComponent->BindAction(DashInputAction, ETriggerEvent::Started, this, &APlayableCharacter::RequestDash);

	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &APlayableCharacter::ClearMovementIntent);
	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &APlayableCharacter::ClearMovementIntent);
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

	MovementIntent = Foward * Input.Y + Right * Input.X;
	// 현재 이동 입력을 대시의 단계 전환 판정에도 전달한다.
	if (IsValid(DashComponent))
		DashComponent->UpdateMovementIntent(MovementIntent);

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

// [대시-03] 애님 인스턴스가 추천한 변형을 대시 시작 요청으로 전달한다.
void APlayableCharacter::RequestDash()
{
	if (!IsValid(DashComponent))
	{
		return;
	}

	UPlayableAnimInstance* AnimInstance = Cast<UPlayableAnimInstance>(GetMesh()->GetAnimInstance());
	if (!AnimInstance)
	{
		return;
	}

	DashComponent->TryStartDash(AnimInstance->GetDashStartVariant());
}

// [대시-03] 이동키를 놓으면 이동 의도를 초기화하고 대시에도 입력 해제를 전달한다.
void APlayableCharacter::ClearMovementIntent()
{
	MovementIntent = FVector::ZeroVector;

	if (IsValid(DashComponent))
		DashComponent->UpdateMovementIntent(MovementIntent);
}
