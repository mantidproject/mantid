// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H

#include "CatalogRunNotifier.h"
#include "Common/DllConfig.h"
#include "GUI/RunsTable/IRunsTablePresenter.h"
#include "GUI/RunsTable/RunsTablePresenterFactory.h"
#include "IRunsPresenter.h"
#include "IRunsView.h"
#include "ISearcher.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IAlgorithm.h"
#include "SearchResult.h"
#include <boost/shared_ptr.hpp>

class ProgressPresenter;

namespace MantidQt {

namespace MantidWidgets {
// Forward decs
class ProgressableView;
} // namespace MantidWidgets

namespace CustomInterfaces {
namespace ISISReflectometry {

// Forward decs
class IMessageHandler;
class IPythonRunner;

using MantidWidgets::ProgressableView;

enum class TransferMatch {
  Any,        // any that match the regex
  ValidTheta, // any that match and have a valid theta value
  Strict      // only those that exactly match all parts of the regex
};

/** @class RunsPresenter

RunsPresenter is a presenter class for the Reflectometry Interface. It
handles any interface functionality and model manipulation.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL RunsPresenter
    : public IRunsPresenter,
      public RunsViewSubscriber,
      public RunNotifierSubscriber,
      public SearcherSubscriber,
      public Mantid::API::AlgorithmObserver {
public:
  RunsPresenter(IRunsView *mainView, ProgressableView *progressView,
                const RunsTablePresenterFactory &makeRunsTablePresenter,
                double thetaTolerance,
                std::vector<std::string> const &instruments,
                int defaultInstrumentIndex, IMessageHandler *messageHandler);
  RunsPresenter(RunsPresenter const &) = delete;
  ~RunsPresenter() override;
  RunsPresenter const &operator=(RunsPresenter const &) = delete;

  RunsPresenter(RunsPresenter &&) = default;
  RunsPresenter &operator=(RunsPresenter &&) = default;

  // IRunsPresenter overrides
  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  RunsTable const &runsTable() const override;
  RunsTable &mutableRunsTable() override;
  bool isProcessing() const override;
  bool isAutoreducing() const override;
  int percentComplete() const override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void notifyReductionResumed() override;
  void notifyReductionPaused() override;
  void notifyRowStateChanged() override;
  void notifyRowStateChanged(boost::optional<Item const &> item) override;
  void notifyRowOutputsChanged() override;
  void notifyRowOutputsChanged(boost::optional<Item const &> item) override;

  void reductionPaused() override;
  void reductionResumed() override;
  bool resumeAutoreduction() override;
  void autoreductionResumed() override;
  void autoreductionPaused() override;
  void autoreductionCompleted() override;
  void anyBatchAutoreductionResumed() override;
  void anyBatchAutoreductionPaused() override;
  void instrumentChanged(std::string const &instrumentName) override;
  void settingsChanged() override;

  // RunsViewSubscriber overrides
  void notifySearch() override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyTransfer() override;
  void notifyInstrumentChanged() override;
  void notifyStartMonitor() override;
  void notifyStopMonitor() override;
  void notifyStartMonitorComplete() override;

  // RunNotifierSubscriber overrides
  void notifyCheckForNewRuns() override;

  // SearcherSubscriber overrides
  void notifySearchComplete() override;
  void notifySearchFailed() override;

protected:
  IRunsTablePresenter *tablePresenter() const;
  /// The current transfer method
  std::string m_currentTransferMethod;
  /// The data processor presenters stored in a vector
  std::unique_ptr<IRunsTablePresenter> m_tablePresenter;
  /// The run notifier implementation
  std::unique_ptr<IRunNotifier> m_runNotifier;
  /// The search implementation
  std::unique_ptr<ISearcher> m_searcher;

  std::string liveDataReductionOptions(const std::string &inputWorkspace,
                                       const std::string &instrument);

private:
  /// The main view we're managing
  IRunsView *m_view;
  /// The progress view
  ProgressableView *m_progressView;
  /// The main presenter
  IBatchPresenter *m_mainPresenter;
  /// The message reporting implementation
  IMessageHandler *m_messageHandler;
  /// The list of instruments
  std::vector<std::string> m_instruments;
  /// The default index in the instrument list
  int m_defaultInstrumentIndex;
  /// The name to use for the live data workspace
  Mantid::API::IAlgorithm_sptr m_monitorAlg;
  double m_thetaTolerance;

  bool isAnyBatchAutoreducing() const;

  /// searching
  bool search(ISearcher::SearchType searchType);
  void populateSearchResults(Mantid::API::ITableWorkspace_sptr results);
  bool searchInProgress() const;
  /// autoreduction
  bool requireNewAutoreduction() const;
  void checkForNewRuns();
  void autoreduceNewRuns();

  ProgressPresenter setupProgressBar(const std::set<int> &rowsToTransfer);
  void transfer(const std::set<int> &rowsToTransfer,
                const TransferMatch matchType = TransferMatch::Any);
  void updateWidgetEnabledState() const;
  /// Check that a given set of row indices are valid to transfer
  bool validateRowsToTransfer(const std::set<int> &rowsToTransfer);
  /// Get runs to transfer from row indices
  std::vector<SearchResult>
  getSearchResultRunDetails(const std::set<int> &rowsToTransfer);
  /// Get the data for a cell in the search results table as a string
  std::string searchModelData(const int row, const int column);
  /// Start the live data monitor
  void startMonitor();
  void stopMonitor();
  void startMonitorComplete();
  std::string liveDataReductionAlgorithm();
  Mantid::API::IAlgorithm_sptr setupLiveDataMonitorAlgorithm();

  void handleError(const std::string &message, const std::exception &e);
  void handleError(const std::string &message);

  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  void errorHandle(const Mantid::API::IAlgorithm *alg,
                   const std::string &what) override;
  void updateViewWhenMonitorStarting();
  void updateViewWhenMonitorStarted();
  void updateViewWhenMonitorStopped();
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H */
