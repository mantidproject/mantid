// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

enum class InstrumentCategory { POLREF, OTHER };

inline InstrumentCategory instrumentCategory(std::string const &instrumentName) {
  return instrumentName == "POLREF" ? InstrumentCategory::POLREF : InstrumentCategory::OTHER;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
