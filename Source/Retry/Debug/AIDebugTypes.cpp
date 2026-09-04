#include "Debug/AIDebugTypes.h"

FAIMissionDebugSnapshot BuildAIMissionDebugSnapshot(
	const bool bHasMission,
	const FMissionContext& Mission,
	const bool bMissionMovementAllowed,
	const FCommandIntent* CurrentCommand,
	const bool bBlackboardTargetSet,
	const FVector BlackboardTargetLocation,
	const bool bBlackboardMovementAllowed)
{
	FAIMissionDebugSnapshot Snapshot;
	Snapshot.bHasMission = bHasMission;
	Snapshot.bMissionMovementAllowed =
		bHasMission && bMissionMovementAllowed;
	Snapshot.bBlackboardTargetSet = bBlackboardTargetSet;
	Snapshot.BlackboardTargetLocation = BlackboardTargetLocation;
	Snapshot.bBlackboardMovementAllowed = bBlackboardMovementAllowed;

	if (CurrentCommand)
	{
		Snapshot.bHasCommand = true;
		Snapshot.CommandId = CurrentCommand->CommandId.ToString(
			EGuidFormats::DigitsWithHyphens);
		Snapshot.CommandVerb = CurrentCommand->Verb;
		Snapshot.CommandStatus = CurrentCommand->Status;
	}

	if (bHasMission)
	{
		Snapshot.ObjectiveId = Mission.ObjectiveId;
		Snapshot.MissionTargetLocation = Mission.ObjectiveLocation;
		Snapshot.bCommandMatchesMission = CurrentCommand
			&& CurrentCommand->CommandId == Mission.CommandId;
		Snapshot.bBlackboardSynchronized = bBlackboardTargetSet
			&& BlackboardTargetLocation.Equals(
				Mission.ObjectiveLocation, KINDA_SMALL_NUMBER)
			&& bBlackboardMovementAllowed
				== Snapshot.bMissionMovementAllowed;
	}
	else
	{
		Snapshot.bCommandMatchesMission = !CurrentCommand;
		Snapshot.bBlackboardSynchronized = !bBlackboardTargetSet
			&& !bBlackboardMovementAllowed;
	}

	return Snapshot;
}
