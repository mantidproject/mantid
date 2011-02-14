#include "MantidVisitPresenters/ProgressAction.h"

namespace Mantid
{
namespace VATES
{

ProgressAction::ProgressAction()
{
}

void ProgressAction::handler(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf)
{
    this->eventRaised(pNf->progress);
}

}
}

