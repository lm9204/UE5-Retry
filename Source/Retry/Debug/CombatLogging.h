// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RetryCharacter.h"
#include "RetryNPCCharacter.h"

// 로그에 남길 대상 이름. NPC는 부여된 이름(NPCName), 플레이어는 "Player",
// 그 외 액터는 엔진이 생성한 이름을 그대로 쓴다.
inline FString GetCombatLogName(const AActor* Actor)
{
	if (!Actor) return TEXT("None");

	if (const ARetryNPCCharacter* NPC = Cast<ARetryNPCCharacter>(Actor))
		return NPC->NPCName;

	if (Cast<ARetryCharacter>(Actor))
		return TEXT("Player");

	return Actor->GetName();
}
