// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/Instrument/IInstrumentView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockInstrumentView : public IInstrumentView {
public:
  MOCK_METHOD1(subscribe, void(InstrumentViewSubscriber *));
  MOCK_METHOD0(connectInstrumentSettingsWidgets, void());
  MOCK_METHOD0(disconnectInstrumentSettingsWidgets, void());
  MOCK_CONST_METHOD0(getMonitorIndex, int());
  MOCK_METHOD1(setMonitorIndex, void(int));
  MOCK_CONST_METHOD0(getIntegrateMonitors, bool());
  MOCK_METHOD1(setIntegrateMonitors, void(bool));
  MOCK_CONST_METHOD0(getLambdaMin, double());
  MOCK_METHOD1(setLambdaMin, void(double));
  MOCK_CONST_METHOD0(getLambdaMax, double());
  MOCK_METHOD1(setLambdaMax, void(double));
  MOCK_METHOD0(showLambdaRangeInvalid, void());
  MOCK_METHOD0(showLambdaRangeValid, void());
  MOCK_CONST_METHOD0(getMonitorBackgroundMin, double());
  MOCK_METHOD1(setMonitorBackgroundMin, void(double));
  MOCK_CONST_METHOD0(getMonitorBackgroundMax, double());
  MOCK_METHOD1(setMonitorBackgroundMax, void(double));
  MOCK_METHOD0(showMonitorBackgroundRangeInvalid, void());
  MOCK_METHOD0(showMonitorBackgroundRangeValid, void());
  MOCK_CONST_METHOD0(getMonitorIntegralMin, double());
  MOCK_METHOD1(setMonitorIntegralMin, void(double));
  MOCK_CONST_METHOD0(getMonitorIntegralMax, double());
  MOCK_METHOD1(setMonitorIntegralMax, void(double));
  MOCK_METHOD0(showMonitorIntegralRangeInvalid, void());
  MOCK_METHOD0(showMonitorIntegralRangeValid, void());
  MOCK_CONST_METHOD0(getCorrectDetectors, bool());
  MOCK_METHOD1(setCorrectDetectors, void(bool));
  MOCK_CONST_METHOD0(getDetectorCorrectionType, std::string());
  MOCK_METHOD1(setDetectorCorrectionType, void(std::string const &));
  MOCK_METHOD0(disableAll, void());
  MOCK_METHOD0(enableAll, void());
  MOCK_METHOD0(enableDetectorCorrectionType, void());
  MOCK_METHOD0(disableDetectorCorrectionType, void());
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
