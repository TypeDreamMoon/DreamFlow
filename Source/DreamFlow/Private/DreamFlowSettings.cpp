#include "DreamFlowSettings.h"

#define LOCTEXT_NAMESPACE "DreamFlowSettings"

namespace
{
    static ELogVerbosity::Type NormalizeVerbosity(const ELogVerbosity::Type Verbosity)
    {
        return static_cast<ELogVerbosity::Type>(Verbosity & ELogVerbosity::VerbosityMask);
    }
}

const UDreamFlowSettings* UDreamFlowSettings::Get()
{
    return GetDefault<UDreamFlowSettings>();
}

#if WITH_EDITOR
FText UDreamFlowSettings::GetSectionText() const
{
    return LOCTEXT("DreamFlowSectionText", "Dream Flow");
}

FText UDreamFlowSettings::GetSectionDescription() const
{
    return LOCTEXT("DreamFlowSectionDescription", "Configure DreamFlow runtime logging and diagnostics.");
}
#endif

ELogVerbosity::Type UDreamFlowSettings::GetRuntimeLogVerbosity() const
{
    switch (LogVerbosity)
    {
    case EDreamFlowLogVerbositySetting::Off:
        return ELogVerbosity::NoLogging;

    case EDreamFlowLogVerbositySetting::Error:
        return ELogVerbosity::Error;

    case EDreamFlowLogVerbositySetting::Warning:
        return ELogVerbosity::Warning;

    case EDreamFlowLogVerbositySetting::Display:
        return ELogVerbosity::Display;

    case EDreamFlowLogVerbositySetting::Verbose:
        return ELogVerbosity::Verbose;

    case EDreamFlowLogVerbositySetting::VeryVerbose:
        return ELogVerbosity::VeryVerbose;

    case EDreamFlowLogVerbositySetting::Log:
    default:
        return ELogVerbosity::Log;
    }
}

bool UDreamFlowSettings::IsChannelEnabled(const EDreamFlowLogChannel Channel) const
{
    switch (Channel)
    {
    case EDreamFlowLogChannel::General:
        return bLogGeneral;

    case EDreamFlowLogChannel::Execution:
        return bLogExecution;

    case EDreamFlowLogChannel::Variables:
        return bLogVariables;

    case EDreamFlowLogChannel::Replication:
        return bLogReplication;

    case EDreamFlowLogChannel::Validation:
        return bLogValidation;

    case EDreamFlowLogChannel::Tests:
        return bLogAutomationTests;

    default:
        return true;
    }
}

bool UDreamFlowSettings::ShouldLog(const ELogVerbosity::Type Verbosity, const EDreamFlowLogChannel Channel) const
{
    if (!bEnableLogging || !IsChannelEnabled(Channel))
    {
        return false;
    }

    const ELogVerbosity::Type RequestedVerbosity = NormalizeVerbosity(Verbosity);
    const ELogVerbosity::Type ThresholdVerbosity = GetRuntimeLogVerbosity();

    if (ThresholdVerbosity == ELogVerbosity::NoLogging)
    {
        return false;
    }

    return RequestedVerbosity <= ThresholdVerbosity;
}

#undef LOCTEXT_NAMESPACE
