#include "AI/CommandTypes.h"

#define LOCTEXT_NAMESPACE "CommandTypes"

bool IsCommandStatusTerminal(const ECommandStatus Status)
{
	return Status == ECommandStatus::Completed
		|| Status == ECommandStatus::Failed
		|| Status == ECommandStatus::Cancelled;
}

bool CanTransitionCommandStatus(
	const ECommandStatus From, const ECommandStatus To)
{
	if (From == To || IsCommandStatusTerminal(From))
	{
		return false;
	}

	// An issuer may withdraw a command at any point before a terminal state.
	if (To == ECommandStatus::Cancelled)
	{
		return true;
	}

	switch (From)
	{
	case ECommandStatus::Proposed:
		return To == ECommandStatus::Validated;
	case ECommandStatus::Validated:
		return To == ECommandStatus::Assigned;
	case ECommandStatus::Assigned:
		return To == ECommandStatus::Executing;
	case ECommandStatus::Executing:
		return To == ECommandStatus::Completed
			|| To == ECommandStatus::Failed;
	default:
		return false;
	}
}

bool TryTransitionCommandStatus(
	FCommandIntent& Command,
	const ECommandStatus NewStatus,
	FText& OutError)
{
	if (!CanTransitionCommandStatus(Command.Status, NewStatus))
	{
		OutError = FText::Format(
			LOCTEXT("InvalidStatusTransition", "Command status cannot transition from {0} to {1}."),
			FText::FromString(UEnum::GetValueAsString(Command.Status)),
			FText::FromString(UEnum::GetValueAsString(NewStatus)));
		return false;
	}

	Command.Status = NewStatus;
	OutError = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
