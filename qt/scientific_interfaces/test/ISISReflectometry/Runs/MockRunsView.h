// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MOCKBATCHVIEW_H_
#define MANTID_CUSTOMINTERFACES_MOCKBATCHVIEW_H_
#include "DllConfig.h"
#include "GUI/Runs/IRunsView.h"
#include "GUI/RunsTable/IRunsTableView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockRunsView : public IRunsView {
public:
  MockRunsTabView() {
    ON_CALL(*this, table()).WillByDefault(testing::Return(m_tableView));
  }

  // IO
  MOCK_CONST_METHOD0(getSelectedSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getAllSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getSearchString, std::string());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_CONST_METHOD0(getTransferMethod, std::string());
  MOCK_CONST_METHOD0(getAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_CONST_METHOD0(getMonitorAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_CONST_METHOD0(getSelectedGroup, int());
  MOCK_METHOD1(setTransferMethods, void(const std::set<std::string> &));
  MOCK_METHOD0(setTableCommandsProxy, void());
  MOCK_METHOD0(setRowCommandsProxy, void());
  MOCK_METHOD0(clearCommands, void());
  MOCK_METHOD2(setInstrumentList, void(const std::vector<std::string> &, int));
  MOCK_METHOD1(updateMenuEnabledState, void(bool));
  MOCK_METHOD1(setAutoreduceButtonEnabled, void(bool));
  MOCK_METHOD1(setAutoreducePauseButtonEnabled, void(bool));
  MOCK_METHOD1(setTransferButtonEnabled, void(bool));
  MOCK_METHOD1(setInstrumentComboEnabled, void(bool));
  MOCK_METHOD1(subscribe, void(IReflRunsTabPresenter *));
  MOCK_CONST_METHOD0(table, IRunsTableView *());
  MOCK_METHOD1(setSearchTextEntryEnabled, void(bool));
  MOCK_METHOD1(setSearchButtonEnabled, void(bool));
  MOCK_METHOD1(setStartMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(setStopMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(startTimer, void(const int));
  MOCK_METHOD0(stopTimer, void());
  MOCK_METHOD0(startIcatSearch, void());
  MOCK_METHOD0(startMonitor, void());
  MOCK_METHOD0(stopMonitor, void());
  MOCK_METHOD0(updateMonitorRunning, void());
  MOCK_METHOD0(updateMonitorStopped, void());

  // Calls we don't care about
  void showSearch(ReflSearchModel_sptr) override{};
  IReflRunsTabPresenter *getPresenter() const override { return nullptr; };

private:
  IRunsTableView *m_tableView;
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSVIEW_H_
