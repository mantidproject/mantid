// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IRUNSVIEW_H
#define MANTID_ISISREFLECTOMETRY_IRUNSVIEW_H

#include "Common/DllConfig.h"
#include "GUI/RunsTable/IRunsTableView.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include <boost/shared_ptr.hpp>
#include <set>
#include <string>
#include <vector>

namespace MantidQt {

namespace MantidWidgets {
namespace DataProcessor {
class Command;
}
} // namespace MantidWidgets
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

namespace DataProcessor = MantidWidgets::DataProcessor;
class SearchModel;

/**
IRunsView is the base view class for the Reflectometry "Runs"
tab. It contains no QT specific functionality as that should be handled by a
subclass.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL RunsViewSubscriber {
public:
  virtual void notifySearch() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyTransfer() = 0;
  virtual void notifyInstrumentChanged() = 0;
  virtual void notifyStartMonitor() = 0;
  virtual void notifyStopMonitor() = 0;
  virtual void notifyStartMonitorComplete() = 0;
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
};

/** @class IRunsView

IRunsView is the base view class for the Reflectometry Interface. It
contains no QT specific functionality as that should be handled by a subclass.
*/

class MANTIDQT_ISISREFLECTOMETRY_DLL IRunsView
    : public MantidQt::MantidWidgets::ProgressableView {
public:
  virtual ~IRunsView() = default;

  virtual void subscribe(RunsViewSubscriber *notifyee) = 0;
  virtual void subscribeTimer(RunsViewTimerSubscriber *notifyee) = 0;
  virtual void subscribeSearch(RunsViewSearchSubscriber *notifyee) = 0;
  virtual IRunsTableView *table() const = 0;

  // Timer methods
  virtual void startTimer(const int millisecs) = 0;
  virtual void stopTimer() = 0;

  // Connect the model
  virtual void showSearch(boost::shared_ptr<SearchModel> model) = 0;

  // Setter methods
  virtual void setInstrumentList(const std::vector<std::string> &instruments,
                                 int defaultInstrumentIndex) = 0;
  virtual void updateMenuEnabledState(bool isProcessing) = 0;
  virtual void setAutoreduceButtonEnabled(bool enabled) = 0;
  virtual void setAutoreducePauseButtonEnabled(bool enabled) = 0;
  virtual void setTransferButtonEnabled(bool enabled) = 0;
  virtual void setInstrumentComboEnabled(bool enabled) = 0;
  virtual void setSearchTextEntryEnabled(bool enabled) = 0;
  virtual void setSearchButtonEnabled(bool enabled) = 0;
  virtual void setStartMonitorButtonEnabled(bool enabled) = 0;
  virtual void setStopMonitorButtonEnabled(bool enabled) = 0;

  // Accessor methods
  virtual std::set<int> getSelectedSearchRows() const = 0;
  virtual std::set<int> getAllSearchRows() const = 0;
  virtual std::string getSearchInstrument() const = 0;
  virtual void setSearchInstrument(std::string const &instrumentName) = 0;
  virtual std::string getSearchString() const = 0;

  virtual boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const = 0;
  virtual boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getMonitorAlgorithmRunner() const = 0;

  virtual void loginFailed(std::string const &fullError) = 0;
  virtual void noActiveICatSessions() = 0;
  virtual void missingRunsToTransfer() = 0;

  // Start an ICAT search
  virtual void startIcatSearch() = 0;

  // Start live data monitoring
  virtual void startMonitor() = 0;
  virtual void stopMonitor() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IRUNSVIEW_H */
