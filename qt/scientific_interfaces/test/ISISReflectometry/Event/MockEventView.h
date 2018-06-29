#include "../../../ISISReflectometry/GUI/Event/IEventView.h"
#include <gmock/gmock.h>
#include "MantidKernel/WarningSuppressions.h"

GCC_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockEventView : public IEventView {
public:
  MOCK_METHOD1(subscribe, void(EventTabViewSubscriber *));

  MOCK_CONST_METHOD0(logBlockName, std::string());
  MOCK_CONST_METHOD0(logBreakpoints, std::string());
  MOCK_CONST_METHOD0(customBreakpoints, std::string());
  MOCK_CONST_METHOD0(uniformSliceCount, int());
  MOCK_CONST_METHOD0(uniformSliceLength, double());

  MOCK_METHOD0(showCustomBreakpointsInvalid, void());
  MOCK_METHOD0(showCustomBreakpointsValid, void());
  MOCK_METHOD0(showLogBreakpointsInvalid, void());
  MOCK_METHOD0(showLogBreakpointsValid, void());

  MOCK_METHOD1(enableSliceType, void(SliceType));
  MOCK_METHOD1(disableSliceType, void(SliceType));
  MOCK_METHOD0(enableSliceTypeSelection, void());
  MOCK_METHOD0(disableSliceTypeSelection, void());
};
}
}
GCC_DIAG_ON_SUGGEST_OVERRIDE
