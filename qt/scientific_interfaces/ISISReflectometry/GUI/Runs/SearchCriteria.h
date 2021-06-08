// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** @class SearchCriteria

Utility struct to hold the information required for a search
*/
struct MANTIDQT_ISISREFLECTOMETRY_DLL SearchCriteria {
  std::string instrument;
  std::string cycle;
  std::string investigation;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(SearchCriteria const &lhs, SearchCriteria const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(SearchCriteria const &lhs, SearchCriteria const &rhs);
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
