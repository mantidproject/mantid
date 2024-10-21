// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Options/IOptionsDialogPresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
GNU_DIAG_OFF_SUGGEST_OVERRIDE
class MockOptionsDialogPresenter : public IOptionsDialogPresenter {
public:
  MOCK_METHOD0(notifySubscribeView, void());
  MOCK_METHOD1(getBoolOption, bool(const std::string &));
  MOCK_METHOD1(getIntOption, int &(const std::string &));
  MOCK_METHOD0(showView, void());
  MOCK_METHOD1(subscribe, void(OptionsDialogPresenterSubscriber *));

  ~MockOptionsDialogPresenter() override {};
};

class MockOptionsDialogPresenterSubscriber : public OptionsDialogPresenterSubscriber {
public:
  MOCK_CONST_METHOD0(notifyOptionsChanged, void());
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
