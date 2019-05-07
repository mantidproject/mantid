// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H

#include "DllConfig.h"
#include "IReflRunsTabPresenter.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "ReflAutoreduction.h"
#include "ReflTransferStrategy.h"
#include <boost/shared_ptr.hpp>

class ProgressPresenter;

namespace MantidQt {

namespace MantidWidgets {
// Forward decs
class ProgressableView;
namespace DataProcessor {
class DataProcessorPresenter;
}
} // namespace MantidWidgets

namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflRunsTabView;
class IReflSearcher;
class ReflSearchModel;
class ReflTransferStrategy;

using MantidWidgets::DataProcessor::DataProcessorPresenter;
using MantidWidgets::ProgressableView;

/** @class ReflRunsTabPresenter

ReflRunsTabPresenter is a presenter class for the Reflectometry Interface. It
handles any interface functionality and model manipulation.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflRunsTabPresenter
    : public IReflRunsTabPresenter,
      public MantidWidgets::DataProcessor::DataProcessorMainPresenter,
      public Mantid::API::AlgorithmObserver {
public:
  ReflRunsTabPresenter(IReflRunsTabView *mainView,
                       ProgressableView *progressView,
                       std::vector<DataProcessorPresenter *> tablePresenter,
                       boost::shared_ptr<IReflSearcher> searcher =
                           boost::shared_ptr<IReflSearcher>());
  ~ReflRunsTabPresenter() override;
  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;
  void notify(IReflRunsTabPresenter::Flag flag) override;
  void notifyADSChanged(const QSet<QString> &workspaceList, int group) override;
  /// Handle data reduction paused/resumed
  /// Global options (inherited from DataProcessorMainPresenter)
  MantidWidgets::DataProcessor::ColumnOptionsQMap
  getPreprocessingOptions(int group) const override;
  MantidWidgets::DataProcessor::OptionsQMap
  getProcessingOptions(int group) const override;
  QString getPostprocessingOptionsAsString(int group) const override;
  QString getTimeSlicingValues(int group) const override;
  QString getTimeSlicingType(int group) const override;
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle, int group) const override;
  bool hasPerAngleOptions(int group) const override;
  /// Handle data reduction paused/resumed
  void pause(int group) override;
  void resume(int group) const override;
  /// Reduction finished/paused/resumed confirmation handler
  void confirmReductionCompleted(int group) override;
  void confirmReductionPaused(int group) override;
  void confirmReductionResumed(int group) override;
  void settingsChanged(int group) override;
  void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const & /*workspaceName*/) override;
  void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceNames) override;

protected:
  /// Information about the autoreduction process
  ReflAutoreduction m_autoreduction;
  void startNewAutoreduction();
  /// The search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  /// The current transfer method
  std::string m_currentTransferMethod;

private:
  /// The main view we're managing
  IReflRunsTabView *m_view;
  /// The progress view
  ProgressableView *m_progressView;
  /// The data processor presenters stored in a vector
  std::vector<DataProcessorPresenter *> m_tablePresenters;
  /// The main presenter
  IReflMainWindowPresenter *m_mainPresenter;
  /// The search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  /// Legacy transfer method
  static const std::string LegacyTransferMethod;
  /// Measure transfer method
  static const std::string MeasureTransferMethod;
  /// Whether the instrument has been changed before a search was made with it
  bool m_instrumentChanged;
  /// The name to use for the live data workspace
  Mantid::API::IAlgorithm_sptr m_monitorAlg;

  /// searching
  bool search();
  void icatSearchComplete();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  /// autoreduction
  bool requireNewAutoreduction() const;
  bool setupNewAutoreduction(int group, const std::string &searchString);
  void checkForNewRuns();
  void autoreduceNewRuns();
  void pauseAutoreduction();
  void stopAutoreduction();
  int selectedGroup() const;
  int autoreductionGroup() const;
  bool shouldUpdateExistingSearchResults() const;
  bool isAutoreducing(int group) const override;
  bool isAutoreducing() const override;
  // processing
  bool isProcessing(int group) const override;
  bool isProcessing() const override;

  ProgressPresenter setupProgressBar(const std::set<int> &rowsToTransfer);
  void transfer(const std::set<int> &rowsToTransfer, int group,
                const TransferMatch matchType = TransferMatch::Any);
  void pushCommands(int group);
  std::unique_ptr<ReflTransferStrategy> getTransferStrategy();
  void changeInstrument();
  void changeGroup();
  void updateWidgetEnabledState() const;
  DataProcessorPresenter *getTablePresenter(int group) const;
  /// Check that a given set of row indices are valid to transfer
  bool validateRowsToTransfer(const std::set<int> &rowsToTransfer);
  /// Get runs to transfer from row indices
  SearchResultMap
  getSearchResultRunDetails(const std::set<int> &rowsToTransfer);
  /// Deal with rows that are invalid for transfer
  void updateErrorStateInSearchModel(
      const std::set<int> &rowsToTransfer,
      const std::vector<TransferResults::COLUMN_MAP_TYPE> &invalidRuns);
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
#endif /* MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H */
