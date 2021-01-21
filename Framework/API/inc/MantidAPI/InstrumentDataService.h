// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
/** InstrumentDataService Class. Derived from DataService.
Class to store shared_pointer to Instrument Objects.

@author Laurent C Chapon, ISIS, Rutherford Appleton Laboratory
@date 30/05/2008
*/
class MANTID_API_DLL InstrumentDataServiceImpl : public Mantid::Kernel::DataService<Mantid::Geometry::Instrument> {
private:
  friend struct Mantid::Kernel::CreateUsingNew<InstrumentDataServiceImpl>;
  /// Constructor
  InstrumentDataServiceImpl();
  /// Private, unimplemented copy constructor
  InstrumentDataServiceImpl(const InstrumentDataServiceImpl &) = delete;
  /// Private, unimplemented copy assignment operator
  InstrumentDataServiceImpl &operator=(const InstrumentDataServiceImpl &) = delete;
};

using InstrumentDataService = Mantid::Kernel::SingletonHolder<InstrumentDataServiceImpl>;

} // Namespace API
} // Namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::InstrumentDataServiceImpl>;
}
} // namespace Mantid
