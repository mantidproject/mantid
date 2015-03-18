#include "MantidAPI/InstrumentDataService.h"

namespace Mantid {
namespace API {
/**
* Default constructor
*/
InstrumentDataServiceImpl::InstrumentDataServiceImpl()
    : Mantid::Kernel::DataService<Mantid::Geometry::Instrument>(
          "InstrumentDataService") {}

/*
* Destructor
*/
InstrumentDataServiceImpl::~InstrumentDataServiceImpl() {}

} // Namespace API
} // Namespace Mantid
