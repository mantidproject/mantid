// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/Event/IEventView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockEventView : public IEventView {
public:
  MOCK_METHOD1(subscribe, void(EventViewSubscriber *));

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
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
