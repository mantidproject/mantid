// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <initializer_list>
#include <string>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

enum class InstrumentCategory { POLREF, OTHER };

inline InstrumentCategory instrumentCategory(std::string const &instrumentName) {
  return instrumentName == "POLREF" ? InstrumentCategory::POLREF : InstrumentCategory::OTHER;
}

// Apply instrument-specific visibility consistently to both table widgets.
template <typename Table>
void updateInstrumentColumns(Table &table, std::string const &instrumentName,
                             std::initializer_list<std::pair<int, InstrumentCategory>> columns) {
  auto const category = instrumentCategory(instrumentName);
  for (auto const &[column, requiredCategory] : columns)
    table.setColumnHidden(column, category != requiredCategory);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
