// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H

#include "DllConfig.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Runs/IRunsView.h"
#include "GUI/RunsTable/IRunsTablePresenter.h"
#include "GUI/RunsTable/RunsTablePresenterFactory.h"
#include "IReflBatchPresenter.h"
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

// Forward decs
class IReflAutoreduction;
class IReflMessageHandler;
class IReflSearcher;
class ReflSearchModel;

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
      public Mantid::API::AlgorithmObserver {
public:
  RunsPresenter(IRunsView *mainView, ProgressableView *progressView,
                RunsTablePresenterFactory makeRunsTablePresenter,
                double thetaTolerance,
                std::vector<std::string> const &instruments,
                int defaultInstrumentIndex, IReflMessageHandler *messageHandler,
                boost::shared_ptr<IReflAutoreduction> autoreduction =
                    boost::shared_ptr<IReflAutoreduction>(),
                boost::shared_ptr<IReflSearcher> searcher =
                    boost::shared_ptr<IReflSearcher>());
  RunsPresenter(RunsPresenter const &) = delete;
  ~RunsPresenter() override;
  RunsPresenter const &operator=(RunsPresenter const &) = delete;

  RunsPresenter(RunsPresenter &&) = default;
  RunsPresenter &operator=(RunsPresenter &&) = default;

  // IRunsPresenter overrides
  void acceptMainPresenter(IReflBatchPresenter *mainPresenter) override;
  void settingsChanged() override;
  void setInstrumentName(std::string const &instrumentName) override;
  bool isAutoreducing() const override;
  bool isProcessing() const override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void notifyReductionResumed() override;
  void notifyReductionPaused() override;
  void reductionPaused() override;
  void reductionResumed() override;
  void autoreductionResumed() override;
  void autoreductionPaused() override;

  // RunsViewSubscriber overrides
  void notifySearch() override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyTimerEvent() override;
  void notifyICATSearchComplete() override;
  void notifyTransfer() override;
  void notifyInstrumentChanged() override;
  void notifyStartMonitor() override;
  void notifyStopMonitor() override;
  void notifyStartMonitorComplete() override;

protected:
  IRunsTablePresenter *tablePresenter() const;
  /// Information about the autoreduction process
  boost::shared_ptr<IReflAutoreduction> m_autoreduction;
  /// The search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  /// The current transfer method
  std::string m_currentTransferMethod;

private:
  /// The main view we're managing
  IRunsView *m_view;
  /// The progress view
  ProgressableView *m_progressView;
  RunsTablePresenterFactory m_makeRunsTablePresenter;
  /// The data processor presenters stored in a vector
  std::unique_ptr<IRunsTablePresenter> m_tablePresenter;
  /// The main presenter
  IReflBatchPresenter *m_mainPresenter;
  /// The message reporting implementation
  IReflMessageHandler *m_messageHandler;
  /// The search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  /// Whether the instrument has been changed before a search was made with it
  bool m_instrumentChanged;
  /// The name to use for the live data workspace
  Mantid::API::IAlgorithm_sptr m_monitorAlg;
  double m_thetaTolerance;

  /// searching
  bool search();
  void icatSearchComplete();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  /// autoreduction
  bool requireNewAutoreduction() const;
  bool setupNewAutoreduction(const std::string &searchString);
  void checkForNewRuns();
  void autoreduceNewRuns();
  void stopAutoreduction();
  bool shouldUpdateExistingSearchResults() const;

  ProgressPresenter setupProgressBar(const std::set<int> &rowsToTransfer);
  void transfer(const std::set<int> &rowsToTransfer,
                const TransferMatch matchType = TransferMatch::Any);
  void changeGroup();
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
  std::string liveDataReductionOptions(const std::string &instrument);
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
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_RUNSPRESENTER_H */
