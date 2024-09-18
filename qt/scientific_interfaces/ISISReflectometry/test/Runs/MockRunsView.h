// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Runs/IRunsView.h"
#include "GUI/RunsTable/IRunsTableView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MockRunsView : public IRunsView {
public:
  MOCK_METHOD1(subscribe, void(RunsViewSubscriber *));
  MOCK_METHOD1(subscribeTimer, void(RunsViewTimerSubscriber *));
  MOCK_METHOD1(subscribeSearch, void(RunsViewSearchSubscriber *));
  MOCK_CONST_METHOD0(table, IRunsTableView *());
  MOCK_METHOD1(startTimer, void(const int));
  MOCK_METHOD0(stopTimer, void());
  MOCK_METHOD0(resizeSearchResultsColumnsToContents, void());
  MOCK_CONST_METHOD0(getSearchResultsTableWidth, int());
  MOCK_CONST_METHOD1(getSearchResultsColumnWidth, int(int));
  MOCK_METHOD2(setSearchResultsColumnWidth, void(int, int));
  MOCK_CONST_METHOD0(searchResults, ISearchModel const &());
  MOCK_METHOD0(mutableSearchResults, ISearchModel &());

  MOCK_METHOD2(setInstrumentList, void(const std::vector<std::string> &, const std::string &));
  MOCK_METHOD1(updateMenuEnabledState, void(bool));
  MOCK_METHOD1(setAutoreduceButtonEnabled, void(bool));
  MOCK_METHOD1(setAutoreducePauseButtonEnabled, void(bool));
  MOCK_METHOD1(setTransferButtonEnabled, void(bool));
  MOCK_METHOD1(setInstrumentComboEnabled, void(bool));
  MOCK_METHOD1(setSearchTextEntryEnabled, void(bool));
  MOCK_METHOD1(setSearchButtonEnabled, void(bool));
  MOCK_METHOD1(setSearchResultsEnabled, void(bool));
  MOCK_METHOD1(setStartMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(setStopMonitorButtonEnabled, void(bool));
  MOCK_METHOD1(setUpdateIntervalSpinBoxEnabled, void(bool));

  MOCK_METHOD2(setProgressRange, void(int, int));
  MOCK_METHOD1(setProgress, void(int));
  MOCK_METHOD0(clearProgress, void());

  MOCK_CONST_METHOD0(getSelectedSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getAllSearchRows, std::set<int>());
  MOCK_CONST_METHOD0(getSearchInstrument, std::string());
  MOCK_METHOD1(setSearchInstrument, void(std::string const &));
  MOCK_CONST_METHOD0(getSearchString, std::string());
  MOCK_CONST_METHOD0(getSearchCycle, std::string());
  MOCK_CONST_METHOD0(getLiveDataUpdateInterval, int());

  MOCK_CONST_METHOD0(getAlgorithmRunner, std::shared_ptr<MantidQt::API::QtAlgorithmRunner>());
  MOCK_CONST_METHOD0(getMonitorAlgorithmRunner, std::shared_ptr<MantidQt::API::QtAlgorithmRunner>());

  MOCK_METHOD0(startMonitor, void());
  MOCK_METHOD0(stopMonitor, void());
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
