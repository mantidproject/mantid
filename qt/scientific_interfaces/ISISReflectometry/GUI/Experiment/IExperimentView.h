// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H

#include "Common/DllConfig.h"
#include "Common/GetInstrumentParameter.h"
#include "Common/InstrumentParameters.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/Hint.h"
#include "Reduction/PerThetaDefaults.h"
#include <map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IExperimentView

    IExperimentView is the base view class for the Reflectometry experiment
    settings. It
    contains no QT specific functionality as that should be handled by a
   subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentViewSubscriber {
public:
  virtual void notifyPerAngleDefaultsChanged(int column, int row) = 0;
  virtual void notifySettingsChanged() = 0;
  virtual void notifyRestoreDefaultsRequested() = 0;
  virtual void notifySummationTypeChanged() = 0;
  virtual void notifyNewPerAngleDefaultsRequested() = 0;
  virtual void notifyRemovePerAngleDefaultsRequested(int index) = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IExperimentView {
public:
  virtual void subscribe(ExperimentViewSubscriber *notifyee) = 0;
  virtual void connectExperimentSettingsWidgets() = 0;
  virtual void disconnectExperimentSettingsWidgets() = 0;
  virtual void
  createStitchHints(const std::vector<MantidWidgets::Hint> &hints) = 0;

  virtual std::string getAnalysisMode() const = 0;
  virtual void setAnalysisMode(std::string const &analysisMode) = 0;

  virtual std::string getSummationType() const = 0;
  virtual void setSummationType(std::string const &summationType) = 0;

  virtual std::string getReductionType() const = 0;
  virtual void setReductionType(std::string const &reductionType) = 0;
  virtual void enableReductionType() = 0;
  virtual void disableReductionType() = 0;

  virtual bool getIncludePartialBins() const = 0;
  virtual void setIncludePartialBins(bool enable) = 0;
  virtual void enableIncludePartialBins() = 0;
  virtual void disableIncludePartialBins() = 0;

  virtual bool getDebugOption() const = 0;
  virtual void setDebugOption(bool enable) = 0;

  virtual std::vector<PerThetaDefaults::ValueArray>
  getPerAngleOptions() const = 0;
  virtual void
  setPerAngleOptions(std::vector<PerThetaDefaults::ValueArray> rows) = 0;
  virtual void showPerAngleOptionsAsInvalid(int row, int column) = 0;
  virtual void showPerAngleOptionsAsValid(int row) = 0;
  virtual void showAllPerAngleOptionsAsValid() = 0;
  virtual void showStitchParametersValid() = 0;
  virtual void showStitchParametersInvalid() = 0;

  virtual void enablePolarizationCorrections() = 0;
  virtual void disablePolarizationCorrections() = 0;
  virtual void enableFloodCorrectionInputs() = 0;
  virtual void disableFloodCorrectionInputs() = 0;

  virtual double getTransmissionStartOverlap() const = 0;
  virtual void setTransmissionStartOverlap(double start) = 0;
  virtual double getTransmissionEndOverlap() const = 0;
  virtual void setTransmissionEndOverlap(double end) = 0;
  virtual std::string getTransmissionStitchParams() const = 0;
  virtual void setTransmissionStitchParams(std::string const &params) = 0;
  virtual bool getTransmissionScaleRHSWorkspace() const = 0;
  virtual void setTransmissionScaleRHSWorkspace(bool enable) = 0;
  virtual void showTransmissionRangeInvalid() = 0;
  virtual void showTransmissionRangeValid() = 0;
  virtual void showTransmissionStitchParamsInvalid() = 0;
  virtual void showTransmissionStitchParamsValid() = 0;

  virtual bool getPolarizationCorrectionOption() const = 0;
  virtual void setPolarizationCorrectionOption(bool enable) = 0;

  virtual std::string getFloodCorrectionType() const = 0;
  virtual void setFloodCorrectionType(std::string const &correction) = 0;
  virtual std::string getFloodWorkspace() const = 0;
  virtual void setFloodWorkspace(std::string const &workspace) = 0;

  virtual std::string getStitchOptions() const = 0;
  virtual void setStitchOptions(std::string const &stitchOptions) = 0;

  virtual void showOptionLoadErrors(
      std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
      std::vector<MissingInstrumentParameterValue> const &missingValues) = 0;

  virtual void disableAll() = 0;
  virtual void enableAll() = 0;

  virtual void addPerThetaDefaultsRow() = 0;
  virtual void removePerThetaDefaultsRow(int rowIndex) = 0;

  virtual void showPerAngleThetasNonUnique(double tolerance) = 0;

  virtual ~IExperimentView() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H */
