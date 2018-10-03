#include "../../../ISISReflectometry/GUI/Instrument/IInstrumentView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockInstrumentView : public IInstrumentView {
public:
  MOCK_METHOD1(subscribe, void(InstrumentViewSubscriber *));
  MOCK_CONST_METHOD0(getMonitorIndex, int());
  MOCK_CONST_METHOD0(getIntegrateMonitors, bool());
  MOCK_CONST_METHOD0(getLambdaMin, double());
  MOCK_CONST_METHOD0(getLambdaMax, double());
  MOCK_METHOD0(showLambdaRangeInvalid, void());
  MOCK_METHOD0(showLambdaRangeValid, void());
  MOCK_CONST_METHOD0(getMonitorBackgroundMin, double());
  MOCK_CONST_METHOD0(getMonitorBackgroundMax, double());
  MOCK_METHOD0(showMonitorBackgroundRangeInvalid, void());
  MOCK_METHOD0(showMonitorBackgroundRangeValid, void());
  MOCK_CONST_METHOD0(getMonitorIntegralMin, double());
  MOCK_CONST_METHOD0(getMonitorIntegralMax, double());
  MOCK_METHOD0(showMonitorIntegralRangeInvalid, void());
  MOCK_METHOD0(showMonitorIntegralRangeValid, void());
  MOCK_CONST_METHOD0(getCorrectDetectors, bool());
  MOCK_CONST_METHOD0(getDetectorCorrectionType, std::string());
  MOCK_METHOD0(disableAll, void());
  MOCK_METHOD0(enableAll, void());
  MOCK_METHOD0(enableDetectorCorrectionType, void());
  MOCK_METHOD0(disableDetectorCorrectionType, void());
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
