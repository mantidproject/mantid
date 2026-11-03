// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentSettingsViewState.h"

#include "GUI/Common/InstrumentCategory.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {
namespace {
InstrumentSettingsViewState viewStateFor(InstrumentCategory const category) {
  switch (category) {
  case InstrumentCategory::POLREF:
    return {.detectorCorrectionControlsVisible = false, .specularPixelVisible = true};
  case InstrumentCategory::OTHER:
    return {.detectorCorrectionControlsVisible = true, .specularPixelVisible = false};
  }
  return {};
}
} // namespace

InstrumentSettingsViewState instrumentSettingsViewState(std::string const &instrumentName) {
  return viewStateFor(instrumentCategory(instrumentName));
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
