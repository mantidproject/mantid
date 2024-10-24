// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "DetectorCorrections.h"
#include "MonitorCorrections.h"
#include "RangeInLambda.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class Instrument

    The Instrument model holds all information relating to settings on the
    Instrument Settings tab on the GUI
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Instrument {
public:
  Instrument();
  Instrument(boost::optional<RangeInLambda> wavelengthRange, MonitorCorrections monitorCorrections,
             DetectorCorrections detectorCorrections, std::string calibrationFilePath);

  boost::optional<RangeInLambda> const &wavelengthRange() const;
  bool integratedMonitors() const;
  MonitorCorrections const &monitorCorrections() const;
  DetectorCorrections const &detectorCorrections() const;
  std::string const &calibrationFilePath() const;

  size_t monitorIndex() const;
  boost::optional<RangeInLambda> monitorIntegralRange() const;
  boost::optional<RangeInLambda> monitorBackgroundRange() const;
  bool correctDetectors() const;
  DetectorCorrectionType detectorCorrectionType() const;

private:
  boost::optional<RangeInLambda> m_wavelengthRange;
  MonitorCorrections m_monitorCorrections;
  DetectorCorrections m_detectorCorrections;
  std::string m_calibrationFilePath;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(Instrument const &lhs, Instrument const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(Instrument const &lhs, Instrument const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
