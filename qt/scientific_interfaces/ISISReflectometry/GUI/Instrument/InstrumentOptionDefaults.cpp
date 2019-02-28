// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentOptionDefaults.h"
#include "Common/OptionDefaults.h"
#include "MantidAPI/AlgorithmManager.h"
#include "Reduction/Instrument.h"

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");

Instrument
getInstrumentDefaults(Mantid::Geometry::Instrument_const_sptr instrument) {
  auto defaults = OptionDefaults(instrument);

  auto wavelengthRange =
      RangeInLambda(getValue<double>("WavelengthMin", "LambdaMin"),
                    getValue<double>("WavelengthMax", "LambdaMax"));

  auto monitorIndex = defaults.getIntOrZero("I0MonitorIndex", "I0MonitorIndex");
  if (monitorIndex < 0)
    throw std::invalid_argument("Monitor index cannot be negative");

  auto integrate = defaults.getBoolOrFalse("NormalizeByIntegratedMonitors",
                                           "NormalizeByIntegratedMonitors");
  auto backgroundRange =
      RangeInLambda(defaults.getDoubleOrZero("MonitorBackgroundWavelengthMin",
                                             "MonitorBackgroundMin"),
                    defaults.getDoubleOrZero("MonitorBackgroundWavelengthMax",
                                             "MonitorBackgroundMax"));
  if (!backgroundRange.isValid(true))
    throw std::invalid_argument("Invalid monitor background range");

  auto integralRange =
      RangeInLambda(defaults.getDoubleOrZero("MonitorIntegrationWavelengthMin",
                                             "MonitorIntegralMin"),
                    defaults.getDoubleOrZero("MonitorIntegrationWavelengthMax",
                                             "MonitorIntegralMax"));
  if (!integralRange.isValid(false))
    throw std::invalid_argument("Invalid monitor integral range");

  auto monitorCorrections = MonitorCorrections(monitorIndex, integrate,
                                               backgroundRange, integralRange);

  auto detectorCorrectionString = defaults.getStringOrDefault(
      "DetectorCorrectionType", "DetectorCorrectionType", "VerticalShift");
  auto detectorCorrections = DetectorCorrections(
      defaults.getBoolOrFalse("CorrectDetectors", "CorrectDetectors"),
      detectorCorrectionTypeFromString(detectorCorrectionString));

  return Instrument(std::move(wavelengthRange), std::move(monitorCorrections),
                    std::move(detectorCorrections));
}
} // unnamed namespace

Instrument InstrumentOptionDefaults::get(
    Mantid::Geometry::Instrument_const_sptr instrument) {
  return getInstrumentDefaults(instrument);
}
} // namespace CustomInterfaces
} // namespace MantidQt
