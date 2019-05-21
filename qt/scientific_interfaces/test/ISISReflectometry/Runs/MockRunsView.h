// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MOCKRUNSVIEW_H_
#define MANTID_CUSTOMINTERFACES_MOCKRUNSVIEW_H_
#include "GUI/Runs/IRunsView.h"
#include "GUI/RunsTable/IRunsTableView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockRunsView : public IRunsView {
public:
  MOCK_METHOD1(subscribe, void(RunsViewSubscriber *));
  MOCK_CONST_METHOD0(table, IRunsTableView *());

  MOCK_METHOD2(setInstrumentList, void(const std::vector<std::string> &, int));
  MOCK_METHOD1(updateMenuEnabledState, void(bool));
  MOCK_METHOD1(setAutoreduceButtonEnabled, void(bool));
  MOCK_METHOD1(setAutoreducePauseButtonEnabled, void(bool));
  MOCK_METHOD1(setTransferButtonEnabled, void(bool));
  MOCK_METHOD1(setInstrumentComboEnabled, void(bool));
  MOCK_METHOD1(setSearchTextEntryEnabled, void(bool));
  MOCK_METHOD1(setSearchButtonEnabled, void(bool));
  MOCK_METHOD1(setStartMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(setStopMonitorButtonEnabled, void(bool));

  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD0(clearProgress, void());
  MOCK_METHOD1(loginFailed, void(std::string const &));

  MOCK_CONST_METHOD0(getSelectedSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getAllSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_METHOD1(setSearchInstrument, void(std::string const &));
  MOCK_CONST_METHOD0(getSearchString, std::string());

  MOCK_CONST_METHOD0(getAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());
  MOCK_CONST_METHOD0(getMonitorAlgorithmRunner,
                     boost::shared_ptr<MantidQt::API::AlgorithmRunner>());

  MOCK_METHOD1(startTimer, void(const int));
  MOCK_METHOD0(stopTimer, void());

  MOCK_METHOD0(startIcatSearch, void());
  MOCK_METHOD0(noActiveICatSessions, void());
  MOCK_METHOD0(missingRunsToTransfer, void());

  MOCK_METHOD0(startMonitor, void());
  MOCK_METHOD0(stopMonitor, void());

  // Calls we don't care about
  void showSearch(SearchModel_sptr) override{};
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSVIEW_H_
