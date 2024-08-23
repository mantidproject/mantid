// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/RunsTable/IRunsTableView.h"
#include "ISearchModel.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace MantidQt {

namespace API {
class QtAlgorithmRunner;
}

namespace CustomInterfaces {
namespace ISISReflectometry {

/**
IRunsView is the base view class for the Reflectometry "Runs"
tab. It contains no QT specific functionality as that should be handled by a
subclass.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL RunsViewSubscriber {
public:
  virtual void notifySearch() = 0;
  virtual void notifyResumeAutoreductionRequested() = 0;
  virtual void notifyPauseAutoreductionRequested() = 0;
  virtual void notifyTransfer() = 0;
  virtual void notifyChangeInstrumentRequested() = 0;
  virtual void notifyStartMonitor() = 0;
  virtual void notifyStopMonitor() = 0;
  virtual void notifyStartMonitorComplete() = 0;
  virtual void notifyExportSearchResults() const = 0;
};

class RunsViewTimerSubscriber {
public:
  virtual ~RunsViewTimerSubscriber() = default;
  virtual void notifyTimerEvent() = 0;
};

class RunsViewSearchSubscriber {
public:
  virtual ~RunsViewSearchSubscriber() = default;
  virtual void notifySearchComplete() = 0;
  virtual void notifySearchResultsChanged() = 0;
};

/** @class IRunsView

IRunsView is the base view class for the Reflectometry Interface. It
contains no QT specific functionality as that should be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL IRunsView : public MantidQt::MantidWidgets::ProgressableView {
public:
  virtual ~IRunsView() = default;

  virtual void subscribe(RunsViewSubscriber *notifyee) = 0;
  virtual void subscribeTimer(RunsViewTimerSubscriber *notifyee) = 0;
  virtual void subscribeSearch(RunsViewSearchSubscriber *notifyee) = 0;
  virtual IRunsTableView *table() const = 0;

  // Timer methods
  virtual void startTimer(const int millisecs) = 0;
  virtual void stopTimer() = 0;

  // Search methods
  virtual void resizeSearchResultsColumnsToContents() = 0;
  virtual int getSearchResultsTableWidth() const = 0;
  virtual int getSearchResultsColumnWidth(int column) const = 0;
  virtual void setSearchResultsColumnWidth(int column, int width) = 0;
  virtual ISearchModel const &searchResults() const = 0;
  virtual ISearchModel &mutableSearchResults() = 0;

  // Setter methods
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 const std::string &selectedInstrument = "") = 0;
  virtual void updateMenuEnabledState(bool isProcessing) = 0;
  virtual void setAutoreduceButtonEnabled(bool enabled) = 0;
  virtual void setAutoreducePauseButtonEnabled(bool enabled) = 0;
  virtual void setTransferButtonEnabled(bool enabled) = 0;
  virtual void setInstrumentComboEnabled(bool enabled) = 0;
  virtual void setSearchTextEntryEnabled(bool enabled) = 0;
  virtual void setSearchButtonEnabled(bool enabled) = 0;
  virtual void setSearchResultsEnabled(bool enabled) = 0;
  virtual void setSearchInstrument(std::string const &instrumentName) = 0;
  virtual void setStartMonitorButtonEnabled(bool enabled) = 0;
  virtual void setStopMonitorButtonEnabled(bool enabled) = 0;
  virtual void setUpdateIntervalSpinBoxEnabled(bool enabled) = 0;

  // Accessor methods
  virtual std::set<int> getSelectedSearchRows() const = 0;
  virtual std::set<int> getAllSearchRows() const = 0;
  virtual std::string getSearchInstrument() const = 0;
  virtual std::string getSearchString() const = 0;
  virtual std::string getSearchCycle() const = 0;
  virtual int getLiveDataUpdateInterval() const = 0;

  virtual std::shared_ptr<MantidQt::API::QtAlgorithmRunner> getAlgorithmRunner() const = 0;
  virtual std::shared_ptr<MantidQt::API::QtAlgorithmRunner> getMonitorAlgorithmRunner() const = 0;

  // Start live data monitoring
  virtual void startMonitor() = 0;
  virtual void stopMonitor() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
