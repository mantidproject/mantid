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
                IMessageHandler *messageHandler);
  RunsPresenter(RunsPresenter const &) = delete;
  ~RunsPresenter() override;
  RunsPresenter const &operator=(RunsPresenter const &) = delete;

  RunsPresenter(RunsPresenter &&) = default;
  RunsPresenter &operator=(RunsPresenter &&) = default;

  // IRunsPresenter overrides
  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void initInstrumentList() override;
  RunsTable const &runsTable() const override;
  RunsTable &mutableRunsTable() override;
  bool isProcessing() const override;
  bool isAutoreducing() const override;
  int percentComplete() const override;
  void setRoundPrecision(int &precision) override;
  void resetRoundPrecision() override;
  void
  notifyChangeInstrumentRequested(std::string const &instrumentName) override;
  void notifyResumeReductionRequested() override;
  void notifyPauseReductionRequested() override;
  void notifyRowStateChanged() override;
  void notifyRowStateChanged(boost::optional<Item const &> item) override;
  void notifyRowOutputsChanged() override;
  void notifyRowOutputsChanged(boost::optional<Item const &> item) override;

  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  bool resumeAutoreduction() override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;
  void autoreductionCompleted() override;
  void notifyAnyBatchReductionResumed() override;
  void notifyAnyBatchReductionPaused() override;
  void notifyAnyBatchAutoreductionResumed() override;
  void notifyAnyBatchAutoreductionPaused() override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void notifyTableChanged() override;
  void settingsChanged() override;

  bool isAnyBatchProcessing() const override;
  bool isAnyBatchAutoreducing() const override;
  bool isOverwritingTablePrevented() const override;
  bool isOverwriteBatchPrevented() const override;

  // RunsViewSubscriber overrides
  void notifySearch() override;
  void notifyResumeAutoreductionRequested() override;
  void notifyPauseAutoreductionRequested() override;
  void notifyTransfer() override;
  void notifyChangeInstrumentRequested() override;
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
  /// The algorithm used when the live data monitor is running
  Mantid::API::IAlgorithm_sptr m_monitorAlg;

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
  /// The tolerance used when looking up settings by theta
  double m_thetaTolerance;

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
  std::string liveDataReductionOptions(const std::string &inputWorkspace,
                                       const std::string &instrument);

  Mantid::API::IAlgorithm_sptr setupLiveDataMonitorAlgorithm();

  void handleError(const std::string &message, const std::exception &e);
  void handleError(const std::string &message);

  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  void errorHandle(const Mantid::API::IAlgorithm *alg,
                   const std::string &what) override;
  void updateViewWhenMonitorStarting();
  void updateViewWhenMonitorStarted();
  void updateViewWhenMonitorStopped();

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H */
