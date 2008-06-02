#include "MantidAPI/AnalysisDataService.h"

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

