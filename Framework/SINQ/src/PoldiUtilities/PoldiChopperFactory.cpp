// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiUtilities/PoldiChopperFactory.h"

#include "MantidSINQ/PoldiUtilities/PoldiBasicChopper.h"

namespace Mantid {
namespace Poldi {

Poldi::PoldiAbstractChopper *PoldiChopperFactory::createChopper(std::string chopperType) {
  UNUSED_ARG(chopperType);

  return new PoldiBasicChopper();
}
} // namespace Poldi
// namespace Poldi
} // namespace Mantid
