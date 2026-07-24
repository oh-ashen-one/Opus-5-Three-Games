// Copyright Opus 5 Three Games. Original work.

#include "SFCharacter.h"

#include "SFDamage.h"
#include "SFBuildComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

ASFCharacter::ASFCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(38.f, 90.f);

	// The pawn does not rotate with the controller; the mesh turns toward movement
	// instead. Standard third-person shooter setup.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 640.f, 0.f);
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->JumpZVelocity = 620.f;
	Move->AirControl = 0.28f;
	Move->BrakingDecelerationWalking = 2200.f;
	Move->GroundFriction = 8.f;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(52.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 320.f;
	CameraBoom->SocketOffset = FVector(0.f, 58.f, 68.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 18.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 90.f;

	BuildComponent = CreateDefaultSubobject<USFBuildComponent>(TEXT("BuildComponent"));
}

namespace
{
	/** Create a named input action of a given value type, owned by Outer. */
	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		return Action;
	}
}

void ASFCharacter::BuildDefaultInput()
{
	if (DefaultMappingContext)
	{
		return;
	}

	MoveAction = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(this, TEXT("IA_Crouch"), EInputActionValueType::Boolean);

	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	// WASD into a single Axis2D. Enhanced Input has no 2D composite primitive, so
	// each key is swizzled/negated into the right component: W is +Y by default,
	// S negates it, and A/D swizzle Y into X (A additionally negated).
	auto* Negate = NewObject<UInputModifierNegate>(DefaultMappingContext);
	auto* SwizzleYXZ = NewObject<UInputModifierSwizzleAxis>(DefaultMappingContext);
	SwizzleYXZ->Order = EInputAxisSwizzle::YXZ;
	auto* NegateForA = NewObject<UInputModifierNegate>(DefaultMappingContext);

	DefaultMappingContext->MapKey(MoveAction, EKeys::W);
	DefaultMappingContext->MapKey(MoveAction, EKeys::S).Modifiers.Add(Negate);
	{
		FEnhancedActionKeyMapping& D = DefaultMappingContext->MapKey(MoveAction, EKeys::D);
		D.Modifiers.Add(SwizzleYXZ);
	}
	{
		FEnhancedActionKeyMapping& A = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
		A.Modifiers.Add(NegateForA);
		A.Modifiers.Add(SwizzleYXZ);
	}

	// Mouse look. Screen Y is inverted relative to pitch, so negate the Y component.
	{
		auto* NegateY = NewObject<UInputModifierNegate>(DefaultMappingContext);
		NegateY->bX = false;
		NegateY->bY = true;
		NegateY->bZ = false;
		DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(NegateY);
	}

	BuildAction = MakeAction(this, TEXT("IA_Build"), EInputActionValueType::Boolean);
	CyclePieceAction = MakeAction(this, TEXT("IA_CyclePiece"), EInputActionValueType::Boolean);

	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	DefaultMappingContext->MapKey(CrouchAction, EKeys::LeftControl);
	DefaultMappingContext->MapKey(BuildAction, EKeys::LeftMouseButton);
	DefaultMappingContext->MapKey(CyclePieceAction, EKeys::Q);
}

void ASFCharacter::BuildPressed(const FInputActionValue& /*Value*/)
{
	if (BuildComponent)
	{
		BuildComponent->TryBuild();
	}
}

void ASFCharacter::CycleBuildPiece(const FInputActionValue& /*Value*/)
{
	if (BuildComponent)
	{
		BuildComponent->CyclePiece(1);
	}
}

void ASFCharacter::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	BuildDefaultInput();

	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

float ASFCharacter::GetGroundSpeed() const
{
	return GetVelocity().Size2D();
}

float ASFCharacter::DesiredMaxSpeed() const
{
	if (bIsCrouched)
	{
		return CrouchSpeed;
	}
	return bWantsToSprint ? SprintSpeed : WalkSpeed;
}

void ASFCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Blend rather than snap, so sprint ramps in instead of popping. This is the
	// single biggest contributor to the controller feeling good.
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = FMath::FInterpTo(Move->MaxWalkSpeed, DesiredMaxSpeed(), DeltaSeconds, SpeedBlendRate);
}

void ASFCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float ImpactSpeed = FMath::Abs(GetVelocity().Z);
	const float Damage = SFDamage::ComputeFallDamage(ImpactSpeed, SafeFallSpeed, LethalFallSpeed, MaxHealth);
	if (Damage <= 0.f)
	{
		return;
	}

	Health = FMath::Max(Health - Damage, 0.f);
	UE_LOG(LogTemp, Verbose, TEXT("Fall damage %.1f at %.0f cm/s, health %.1f"), Damage, ImpactSpeed, Health);
}

void ASFCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}

	// Move relative to where the camera is looking, flattened to the ground plane.
	const FRotator YawOnly(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void ASFCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ASFCharacter::SprintStart(const FInputActionValue& /*Value*/)
{
	bWantsToSprint = true;
	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void ASFCharacter::SprintStop(const FInputActionValue& /*Value*/)
{
	bWantsToSprint = false;
}

void ASFCharacter::CrouchToggle(const FInputActionValue& /*Value*/)
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		bWantsToSprint = false;
		Crouch();
	}
}

void ASFCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// SetupPlayerInputComponent can run before BeginPlay on possession, so the
	// actions must exist by now rather than being built only in BeginPlay.
	BuildDefaultInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASFCharacter::Move);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASFCharacter::Look);
	}
	if (JumpAction)
	{
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (SprintAction)
	{
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ASFCharacter::SprintStart);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASFCharacter::SprintStop);
	}
	if (CrouchAction)
	{
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASFCharacter::CrouchToggle);
	}
	if (BuildAction)
	{
		Input->BindAction(BuildAction, ETriggerEvent::Started, this, &ASFCharacter::BuildPressed);
	}
	if (CyclePieceAction)
	{
		Input->BindAction(CyclePieceAction, ETriggerEvent::Started, this, &ASFCharacter::CycleBuildPiece);
	}
}
