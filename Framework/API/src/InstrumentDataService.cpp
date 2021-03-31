// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/InstrumentDataService.h"

namespace Mantid {
namespace API {
/**
 * Default constructor
 */
InstrumentDataServiceImpl::InstrumentDataServiceImpl()
    : Mantid::Kernel::DataService<Mantid::Geometry::Instrument>("InstrumentDataService") {}

} // Namespace API
} // Namespace Mantid
