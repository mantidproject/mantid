// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTER_H_
#include "DllConfig.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
class MockRunsPresenter : public IRunsPresenter {
public:
  MOCK_CONST_METHOD0(isAutoreducing, bool());
  MOCK_METHOD0(settingsChanged, void());

  void notify(IRunsPresenter::Flag flag) override { UNUSED_ARG(flag); };
  void acceptMainPresenter(IBatchPresenter *presenter) override {
    UNUSED_ARG(presenter);
  }
  bool isProcessing() const override { return false; }
  ~MockRunsPresenter() override{};
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSPRESENTER_H_
