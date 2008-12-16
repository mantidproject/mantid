#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceTracer.h"

namespace Mantid
{
namespace API
{
AnalysisDataServiceImpl::AnalysisDataServiceImpl()
:Mantid::Kernel::DataService<Mantid::API::Workspace>("AnalysisDataService")
{
  notificationCenter.addObserver(WorkspaceTracer::Instance().m_wkspAftReplaceObserver);
}
AnalysisDataServiceImpl::~AnalysisDataServiceImpl()
{
	
}

} // Namespace API
} // Namespace Mantid

