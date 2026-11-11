// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentSettingsViewState.h"

#include <unordered_map>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
namespace {
enum class InstrumentSettingsCategory { POLREF, OTHER };

InstrumentSettingsCategory categoryFor(std::string const &instrumentName) {
  static auto const categories =
      std::unordered_map<std::string, InstrumentSettingsCategory>{{"POLREF", InstrumentSettingsCategory::POLREF}};
  auto const category = categories.find(instrumentName);
  return category == categories.cend() ? InstrumentSettingsCategory::OTHER : category->second;
}

InstrumentSettingsViewState viewStateFor(InstrumentSettingsCategory const category) {
  switch (category) {
  case InstrumentSettingsCategory::POLREF:
    return {.detectorCorrectionControlsVisible = false, .specularPixelVisible = true};
  case InstrumentSettingsCategory::OTHER:
    return {.detectorCorrectionControlsVisible = true, .specularPixelVisible = false};
  }
  return {};
}
} // namespace

InstrumentSettingsViewState instrumentSettingsViewState(std::string const &instrumentName) {
  return viewStateFor(categoryFor(instrumentName));
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
