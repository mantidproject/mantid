// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/MainWindow/IMainWindowView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class MockMainWindowView : public IMainWindowView {
public:
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD0(newBatch, IBatchView *());
  MOCK_METHOD1(subscribe, void(MainWindowSubscriber *));
  MOCK_METHOD1(removeBatch, void(int));
  MOCK_CONST_METHOD0(batches, std::vector<IBatchView *>());
  ~MockMainWindowView() override{};
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
