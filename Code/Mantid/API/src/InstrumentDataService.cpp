#include "MantidAPI/InstrumentDataService.h"

namespace Mantid
{
namespace API
{
InstrumentDataServiceImpl::InstrumentDataServiceImpl()
:Mantid::Kernel::DataService<Mantid::API::Instrument>("InstrumentDataService")
{
}
InstrumentDataServiceImpl::~InstrumentDataServiceImpl()
{
	
}

} // Namespace API
} // Namespace Mantid
