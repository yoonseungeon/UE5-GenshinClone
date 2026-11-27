#pragma once

#include "CoreMinimal.h"
#include "PlayableLocomotionTypes.generated.h"

// [점프-09] 점프 시작 시 선택할 애니메이션 종류다.
UENUM(BlueprintType)
enum class EJumpClipType : uint8
{
	Standing,
	Run,
	Sprint
};

// [점프-10] 착지 상태에 진입할 때 선택할 애니메이션 종류다.
UENUM(BlueprintType)
enum class ELandingClipType : uint8
{
	Standing,
	Run,
	Sprint
};

// [지상이동-01] 이동 경로의 기본 선택과 다음 RunCycle의 선택을 묶는다.
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FMovementVariantPlan
{
	GENERATED_BODY()

	// Run으로 들어올 경로에서 준비한 선택이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bRunCycleIsMirrored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintIsMirrored = false;

	// [점프-15] 다음 SprintCycle 진입에서 확정할 원본·미러 선택이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintCycleIsMirrored = false;
};

// 각 상태 진입에서 자기 선택만 확정해 블렌딩 중인 다른 상태의 선택을 유지한다.
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FLocomotionVariants
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bRunCycleIsMirrored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bRunStopIsMirrored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintBSIsMirrored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintCycleIsMirrored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintToRunIsMirrored = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bSprintStopIsMirrored = false;

	// [점프-12] 이번 Jump에서 재생할 원본·미러 선택이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bJumpIsMirrored = false;

	// [점프-13] 현재 Landing에서 재생할 원본·미러 선택이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bLandingIsMirrored = false;
};

// 애니메이션 노드가 초기화되기 전에 준비하는 시작 위치다.
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FLocomotionStartTimes
{
	GENERATED_BODY()

	// 다음 Run 진입 전에 앞 상태에서 준비한다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion", meta = (Units = "s"))
	float RunCycle = 0.0f;

	// [점프-15] Sprint 진입 전에 출발 경로에서 준비하는 시작 시각이다.
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion", meta = (Units = "s"))
	float SprintCycle = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (Units = "s"))
	float SprintToRun = 14.0f / 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (Units = "s"))
	float SprintStop = 20.0f / 60.0f;
};

// SprintBS의 정지·재입력과 Run 복귀를 허용하는 재생 시각이다.
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FLocomotionTransitionTimes
{
	GENERATED_BODY()

	// SprintBS 20.44프레임부터 정지와 재입력을 허용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing", meta = (Units = "s"))
	float SprintBSStopAllowedTime = 20.44f / 60.0f;

	// SprintBS 50프레임부터 SprintToRun 전환을 허용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing", meta = (Units = "s"))
	float SprintBSToRunAllowedTime = 50.0f / 60.0f;

	// [점프-11] Run·Sprint 착지에서 정지 전환을 허용하는 재생 시각이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing", meta = (Units = "s"))
	float RunLandingStopAllowedTime = 0.312f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Timing", meta = (Units = "s"))
	float SprintLandingStopAllowedTime = 0.169682f;
};

// 대시 승인 번호의 최신 값과 대기 요청, 진입 완료 번호를 보관한다.
struct GENSHIN_CLONE_API FDashRequestState
{
	int32 SnapshotId = 0;
	int32 PendingId = 0;
	int32 LastEnteredId = 0;
};
