// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "Common/InstrumentParameters.h"
#include "MantidAPI/Algorithm.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class IInstrumentView

IInstrumentView is the base view class for the Reflectometry instrument
settings. It
contains no QT specific functionality as that should be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentViewSubscriber {
public:
  virtual void notifySettingsChanged() = 0;
  virtual void notifyRestoreDefaultsRequested() = 0;
  virtual void notifyBrowseToCalibrationFileRequested() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IInstrumentView {
public:
  virtual ~IInstrumentView() = default;

  virtual void subscribe(InstrumentViewSubscriber *notifyee) = 0;
  virtual void connectInstrumentSettingsWidgets() = 0;
  virtual void disconnectInstrumentSettingsWidgets() = 0;

  virtual int getMonitorIndex() const = 0;
  virtual void setMonitorIndex(int value) = 0;
  virtual bool getIntegrateMonitors() const = 0;
  virtual void setIntegrateMonitors(bool value) = 0;

  virtual double getLambdaMin() const = 0;
  virtual void setLambdaMin(double value) = 0;
  virtual double getLambdaMax() const = 0;
  virtual void setLambdaMax(double value) = 0;
  virtual void showLambdaRangeInvalid() = 0;
  virtual void showLambdaRangeValid() = 0;

  virtual double getMonitorBackgroundMin() const = 0;
  virtual void setMonitorBackgroundMin(double value) = 0;
  virtual double getMonitorBackgroundMax() const = 0;
  virtual void setMonitorBackgroundMax(double value) = 0;
  virtual void showMonitorBackgroundRangeInvalid() = 0;
  virtual void showMonitorBackgroundRangeValid() = 0;

  virtual double getMonitorIntegralMin() const = 0;
  virtual void setMonitorIntegralMin(double value) = 0;
  virtual double getMonitorIntegralMax() const = 0;
  virtual void setMonitorIntegralMax(double value) = 0;
  virtual void showMonitorIntegralRangeInvalid() = 0;
  virtual void showMonitorIntegralRangeValid() = 0;
  virtual void showCalibrationFilePathInvalid() = 0;
  virtual void showCalibrationFilePathValid() = 0;

  virtual bool getCorrectDetectors() const = 0;
  virtual void setCorrectDetectors(bool value) = 0;
  virtual std::string getDetectorCorrectionType() const = 0;
  virtual void setDetectorCorrectionType(std::string const &value) = 0;

  virtual std::string getCalibrationFilePath() const = 0;
  virtual void setCalibrationFilePath(std::string const &value) = 0;

  virtual void disableAll() = 0;
  virtual void enableAll() = 0;
  virtual void enableDetectorCorrectionType() = 0;
  virtual void disableDetectorCorrectionType() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
