// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <SearchCriteria.h>
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

bool operator==(SearchCriteria const &lhs, SearchCriteria const &rhs) {
  return lhs.instrument == rhs.instrument && lhs.cycle == rhs.cycle && lhs.investigation == rhs.investigation;
}

bool operator!=(SearchCriteria const &lhs, SearchCriteria const &rhs) { return !(lhs == rhs); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
