#include "DreamFlowEdGraph.h"

#include "DreamFlowAsset.h"

UDreamFlowAsset* UDreamFlowEdGraph::GetFlowAsset() const
{
    return FlowAsset;
}

void UDreamFlowEdGraph::SetFlowAsset(UDreamFlowAsset* InFlowAsset)
{
    FlowAsset = InFlowAsset;
}
