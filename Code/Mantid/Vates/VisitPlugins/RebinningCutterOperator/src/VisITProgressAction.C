
#include "VisITProgressAction.h"
#include "avtRebinningCutterFilter.h"

namespace Mantid
{
namespace VATES
{

VisITProgressAction::VisITProgressAction(avtRebinningCutterFilter* filter) : m_filter(filter)
{
}

void VisITProgressAction::eventRaised(int progressPercent)
{
  m_filter->UpdateAlgorithmProgress(progressPercent);
}

}
}
