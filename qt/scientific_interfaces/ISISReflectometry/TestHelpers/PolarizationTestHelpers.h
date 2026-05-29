// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "PolarizationCorrections.h"
namespace MantidQt::CustomInterfaces::ISISReflectometry {

bool operator!=(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs) { return !(lhs == rhs); }

bool operator==(PolarizationCorrections const &lhs, PolarizationCorrections const &rhs) {
  return lhs.correctionType() == rhs.correctionType() && lhs.workspace() == rhs.workspace() &&
         lhs.inputSpinStateOrder() == rhs.inputSpinStateOrder();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
