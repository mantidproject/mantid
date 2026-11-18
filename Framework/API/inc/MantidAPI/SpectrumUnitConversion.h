// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Unit.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Mantid::API {

/// What is common to converting between two units for every spectrum of a workspace. The referenced objects must
/// outlive this object.
struct UnitConversionSetup {
  const SpectrumInfo &spectrumInfo;
  const Kernel::Unit &from;
  const Kernel::Unit &to;
  Kernel::DeltaEMode::Type emode;
  bool signedTheta{false};
  /// Parameters known before the detector values are read. A value given for efixed takes precedence over the workspace
  Kernel::UnitParametersMap initialParams{};
};

/**
 * The unit copies and parameters for converting one spectrum. Converting changes the state of a unit, so create one of
 * these per spectrum and per thread. This is called for every spectrum by algorithms such as ConvertUnits, so it is
 * inline and is constructed in place. The setup must outlive this object.
 */
class SpectrumUnitConversion {
public:
  explicit SpectrumUnitConversion(const UnitConversionSetup &setup)
      : from(setup.from.clone()), to(setup.to.clone()), params(setup.initialParams), m_setup(setup) {}

  SpectrumUnitConversion(const SpectrumUnitConversion &) = delete;
  SpectrumUnitConversion &operator=(const SpectrumUnitConversion &) = delete;

  /// Add the values for the detectors of a spectrum to params
  void readDetectorValues(const std::size_t index) {
    m_setup.spectrumInfo.getDetectorValues(m_setup.from, m_setup.to, m_setup.emode, m_setup.signedTheta,
                                           static_cast<int64_t>(index), params);
  }

  std::unique_ptr<Kernel::Unit> from;
  std::unique_ptr<Kernel::Unit> to;
  Kernel::UnitParametersMap params;

private:
  const UnitConversionSetup &m_setup;
};

} // namespace Mantid::API
