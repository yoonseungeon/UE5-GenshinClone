// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AnimNotify_FinishRootMotionJump.h"

#include "Characters/PlayableCharacter.h"
#include "Components/PlayableMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

// [점프-08] Jump 종료 알림에서 루트 점프를 끝내고 중력 낙하로 전환한다.
void UAnimNotify_FinishRootMotionJump::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(MeshComp->GetOwner());
	if (!PlayableCharacter)
		return;

	UPlayableMovementComponent* PlayableMovementComponent = static_cast<UPlayableMovementComponent*>(PlayableCharacter->GetCharacterMovement());
	PlayableMovementComponent->FinishRootMotionJump();
}
