#include "DreamFlowLog.h"

DEFINE_LOG_CATEGORY(LogDreamFlow);

bool FDreamFlowLog::ShouldLog(const ELogVerbosity::Type Verbosity, const EDreamFlowLogChannel Channel)
{
    if (const UDreamFlowSettings* Settings = UDreamFlowSettings::Get())
    {
        return Settings->ShouldLog(Verbosity, Channel);
    }

    return false;
}

const TCHAR* FDreamFlowLog::GetChannelLabel(const EDreamFlowLogChannel Channel)
{
    switch (Channel)
    {
    case EDreamFlowLogChannel::General:
        return TEXT("General");

    case EDreamFlowLogChannel::Execution:
        return TEXT("Execution");

    case EDreamFlowLogChannel::Variables:
        return TEXT("Variables");

    case EDreamFlowLogChannel::Replication:
        return TEXT("Replication");

    case EDreamFlowLogChannel::Validation:
        return TEXT("Validation");

    case EDreamFlowLogChannel::Tests:
        return TEXT("Tests");

    default:
        return TEXT("DreamFlow");
    }
}
