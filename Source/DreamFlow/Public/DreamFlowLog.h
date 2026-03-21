#pragma once

#include "DreamFlowSettings.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDreamFlow, Log, All);

class DREAMFLOW_API FDreamFlowLog
{
public:
    static bool ShouldLog(ELogVerbosity::Type Verbosity, EDreamFlowLogChannel Channel);
    static const TCHAR* GetChannelLabel(EDreamFlowLogChannel Channel);
};

#define DREAMFLOW_LOG(Channel, Verbosity, Format, ...) \
    do \
    { \
        if (FDreamFlowLog::ShouldLog(ELogVerbosity::Verbosity, EDreamFlowLogChannel::Channel)) \
        { \
            UE_LOG(LogDreamFlow, Verbosity, TEXT("[%s] " Format), FDreamFlowLog::GetChannelLabel(EDreamFlowLogChannel::Channel), ##__VA_ARGS__); \
        } \
    } while (false)
