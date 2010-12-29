#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceTracer.h"

namespace Mantid
{
namespace API
{
AnalysisDataServiceImpl::AnalysisDataServiceImpl()
:Mantid::Kernel::DataService<Mantid::API::Workspace>("AnalysisDataService")
{
}
AnalysisDataServiceImpl::~AnalysisDataServiceImpl()
{
	
}

} // Namespace API
} // Namespace Mantid

