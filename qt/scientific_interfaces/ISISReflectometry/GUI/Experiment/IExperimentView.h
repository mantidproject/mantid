// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "Common/GetInstrumentParameter.h"
#include "Common/InstrumentParameters.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/Hint.h"
#include "Reduction/LookupRow.h"
#include <map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class IExperimentView

    IExperimentView is the base view class for the Reflectometry experiment
    settings. It
    contains no QT specific functionality as that should be handled by a
   subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentViewSubscriber {
public:
  virtual void notifyLookupRowChanged(int column, int row) = 0;
  virtual void notifySettingsChanged() = 0;
  virtual void notifyRestoreDefaultsRequested() = 0;
  virtual void notifySummationTypeChanged() = 0;
  virtual void notifyNewLookupRowRequested() = 0;
  virtual void notifyRemoveLookupRowRequested(int index) = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IExperimentView {
public:
  virtual void subscribe(ExperimentViewSubscriber *notifyee) = 0;
  virtual void connectExperimentSettingsWidgets() = 0;
  virtual void disconnectExperimentSettingsWidgets() = 0;
  virtual void createStitchHints(const std::vector<MantidWidgets::Hint> &hints) = 0;

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

  virtual std::vector<LookupRow::ValueArray> getLookupTable() const = 0;
  virtual void setLookupTable(std::vector<LookupRow::ValueArray> rows) = 0;
  virtual void showLookupRowAsInvalid(int row, int column) = 0;
  virtual void showLookupRowAsValid(int row) = 0;
  virtual void showAllLookupRowsAsValid() = 0;
  virtual void showStitchParametersValid() = 0;
  virtual void showStitchParametersInvalid() = 0;
  virtual void showPolCorrFilePathValid() = 0;
  virtual void showPolCorrFilePathInvalid() = 0;
  virtual void showFloodCorrFilePathValid() = 0;
  virtual void showFloodCorrFilePathInvalid() = 0;

  virtual bool getSubtractBackground() const = 0;
  virtual void setSubtractBackground(bool enable) = 0;
  virtual std::string getBackgroundSubtractionMethod() const = 0;
  virtual void setBackgroundSubtractionMethod(std::string const &method) = 0;
  virtual void enableBackgroundSubtractionMethod() = 0;
  virtual void disableBackgroundSubtractionMethod() = 0;
  virtual int getPolynomialDegree() const = 0;
  virtual void setPolynomialDegree(int polynomialDegree) = 0;
  virtual void enablePolynomialDegree() = 0;
  virtual void disablePolynomialDegree() = 0;
  virtual std::string getCostFunction() const = 0;
  virtual void setCostFunction(std::string const &costFunction) = 0;
  virtual void enableCostFunction() = 0;
  virtual void disableCostFunction() = 0;
  virtual void enablePolarizationCorrections() = 0;
  virtual void disablePolarizationCorrections() = 0;
  virtual void enablePolarizationEfficiencies() = 0;
  virtual void disablePolarizationEfficiencies() = 0;
  virtual void enableFredrikzeSpinStateOrder() = 0;
  virtual void disableFredrikzeSpinStateOrder() = 0;
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

  virtual std::string getPolarizationCorrectionOption() const = 0;
  virtual void setPolarizationCorrectionOption(std::string const &enable) = 0;
  virtual void setPolarizationEfficienciesWorkspaceMode() = 0;
  virtual void setPolarizationEfficienciesFilePathMode() = 0;
  virtual std::string getPolarizationEfficienciesWorkspace() const = 0;
  virtual std::string getPolarizationEfficienciesFilePath() const = 0;
  virtual void setPolarizationEfficienciesWorkspace(std::string const &workspace) = 0;
  virtual void setPolarizationEfficienciesFilePath(std::string const &filePath) = 0;
  virtual std::string getFredrikzeSpinStateOrder() const = 0;
  virtual void setFredrikzeSpinStateOrder(std::string const &spinStates) = 0;

  virtual std::string getFloodCorrectionType() const = 0;
  virtual void setFloodCorrectionType(std::string const &correction) = 0;
  virtual void setFloodCorrectionWorkspaceMode() = 0;
  virtual void setFloodCorrectionFilePathMode() = 0;
  virtual std::string getFloodWorkspace() const = 0;
  virtual std::string getFloodFilePath() const = 0;
  virtual void setFloodWorkspace(std::string const &workspace) = 0;
  virtual void setFloodFilePath(std::string const &filePath) = 0;

  virtual std::string getStitchOptions() const = 0;
  virtual void setStitchOptions(std::string const &stitchOptions) = 0;

  virtual void disableAll() = 0;
  virtual void enableAll() = 0;

  virtual void addLookupRow() = 0;
  virtual void removeLookupRow(int rowIndex) = 0;

  virtual void setTooltip(int row, int column, std::string const &text) = 0;

  virtual ~IExperimentView() = default;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
