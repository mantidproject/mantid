// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/Experiment/IExperimentView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MockExperimentView : public IExperimentView {
public:
  MockExperimentView() {
    ON_CALL(*this, getAnalysisMode())
        .WillByDefault(testing::Return("PointDetectorAnalysis"));
    ON_CALL(*this, getSummationType())
        .WillByDefault(testing::Return("SumInLambda"));
    ON_CALL(*this, getReductionType()).WillByDefault(testing::Return("Normal"));
    ON_CALL(*this, getPolarizationCorrectionOption())
        .WillByDefault(testing::Return(false));
    ON_CALL(*this, getFloodCorrectionType())
        .WillByDefault(testing::Return("Workspace"));
    ON_CALL(*this, getDebugOption()).WillByDefault(testing::Return(false));
    ON_CALL(*this, getIncludePartialBins())
        .WillByDefault(testing::Return(false));
  }
  MOCK_METHOD1(subscribe, void(ExperimentViewSubscriber *));
  MOCK_METHOD0(connectExperimentSettingsWidgets, void());
  MOCK_METHOD0(disconnectExperimentSettingsWidgets, void());
  MOCK_METHOD1(createStitchHints,
               void(const std::vector<MantidWidgets::Hint> &));
  MOCK_CONST_METHOD0(getAnalysisMode, std::string());
  MOCK_METHOD1(setAnalysisMode, void(std::string const &));
  MOCK_CONST_METHOD0(getSummationType, std::string());
  MOCK_METHOD1(setSummationType, void(std::string const &));
  MOCK_CONST_METHOD0(getReductionType, std::string());
  MOCK_METHOD1(setReductionType, void(std::string const &));
  MOCK_METHOD0(enableReductionType, void());
  MOCK_METHOD0(disableReductionType, void());
  MOCK_CONST_METHOD0(getIncludePartialBins, bool());
  MOCK_METHOD1(setIncludePartialBins, void(bool));
  MOCK_METHOD0(enableIncludePartialBins, void());
  MOCK_METHOD0(disableIncludePartialBins, void());
  MOCK_CONST_METHOD0(getDebugOption, bool());
  MOCK_METHOD1(setDebugOption, void(bool));
  MOCK_CONST_METHOD0(getPerAngleOptions,
                     std::vector<PerThetaDefaults::ValueArray>());
  MOCK_METHOD1(setPerAngleOptions,
               void(std::vector<PerThetaDefaults::ValueArray>));
  MOCK_METHOD2(showPerAngleOptionsAsInvalid, void(int row, int column));
  MOCK_METHOD1(showPerAngleOptionsAsValid, void(int row));
  MOCK_METHOD0(showAllPerAngleOptionsAsValid, void());
  MOCK_METHOD0(showStitchParametersValid, void());
  MOCK_METHOD0(showStitchParametersInvalid, void());

  MOCK_CONST_METHOD0(getSubtractBackground, bool());
  MOCK_METHOD1(setSubtractBackground, void(bool));
  MOCK_CONST_METHOD0(getBackgroundSubtractionMethod, std::string());
  MOCK_METHOD1(setBackgroundSubtractionMethod, void(std::string const &));
  MOCK_METHOD0(enableBackgroundSubtractionMethod, void());
  MOCK_METHOD0(disableBackgroundSubtractionMethod, void());
  MOCK_CONST_METHOD0(getPolynomialDegree, int());
  MOCK_METHOD1(setPolynomialDegree, void(int));
  MOCK_METHOD0(enablePolynomialDegree, void());
  MOCK_METHOD0(disablePolynomialDegree, void());
  MOCK_CONST_METHOD0(getCostFunction, std::string());
  MOCK_METHOD1(setCostFunction, void(std::string const &));
  MOCK_METHOD0(enableCostFunction, void());
  MOCK_METHOD0(disableCostFunction, void());

  MOCK_METHOD0(enablePolarizationCorrections, void());
  MOCK_METHOD0(disablePolarizationCorrections, void());
  MOCK_METHOD0(enableFloodCorrectionInputs, void());
  MOCK_METHOD0(disableFloodCorrectionInputs, void());
  MOCK_CONST_METHOD0(getTransmissionStartOverlap, double());
  MOCK_METHOD1(setTransmissionStartOverlap, void(double));
  MOCK_CONST_METHOD0(getTransmissionEndOverlap, double());
  MOCK_METHOD1(setTransmissionEndOverlap, void(double));
  MOCK_CONST_METHOD0(getTransmissionStitchParams, std::string());
  MOCK_METHOD1(setTransmissionStitchParams, void(std::string const &));
  MOCK_CONST_METHOD0(getTransmissionScaleRHSWorkspace, bool());
  MOCK_METHOD1(setTransmissionScaleRHSWorkspace, void(bool));
  MOCK_METHOD0(showTransmissionRangeValid, void(void));
  MOCK_METHOD0(showTransmissionRangeInvalid, void(void));
  MOCK_METHOD0(showTransmissionStitchParamsValid, void(void));
  MOCK_METHOD0(showTransmissionStitchParamsInvalid, void(void));
  MOCK_CONST_METHOD0(getPolarizationCorrectionOption, bool());
  MOCK_METHOD1(setPolarizationCorrectionOption, void(bool));
  MOCK_CONST_METHOD0(getFloodCorrectionType, std::string());
  MOCK_METHOD1(setFloodCorrectionType, void(std::string const &));
  MOCK_CONST_METHOD0(getFloodWorkspace, std::string());
  MOCK_METHOD1(setFloodWorkspace, void(std::string const &));
  MOCK_CONST_METHOD0(getStitchOptions, std::string());
  MOCK_METHOD1(setStitchOptions, void(std::string const &));
  MOCK_METHOD2(showOptionLoadErrors,
               void(std::vector<InstrumentParameterTypeMissmatch> const &,
                    std::vector<MissingInstrumentParameterValue> const &));
  MOCK_METHOD0(disableAll, void());
  MOCK_METHOD0(enableAll, void());
  MOCK_METHOD0(addPerThetaDefaultsRow, void());
  MOCK_METHOD1(removePerThetaDefaultsRow, void(int));
  MOCK_METHOD1(showPerAngleThetasNonUnique, void(double));
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
