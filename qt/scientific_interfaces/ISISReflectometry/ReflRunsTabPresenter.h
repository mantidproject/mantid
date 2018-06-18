#ifndef MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H

#include "DllConfig.h"
#include "IReflRunsTabPresenter.h"
#include "Presenters/BatchPresenter.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "Presenters/BatchPresenterFactory.h"
#include "ReflAutoreduction.h"
#include <boost/shared_ptr.hpp>
#include "SearchResult.h"

class ProgressPresenter;

namespace MantidQt {

namespace MantidWidgets {
// Forward decs
class ProgressableView;
namespace DataProcessor {
class DataProcessorPresenter;
}
}

namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflRunsTabView;
class IReflSearcher;
class ReflSearchModel;
class ReflTransferStrategy;

using MantidWidgets::ProgressableView;
using MantidWidgets::DataProcessor::DataProcessorPresenter;

enum class TransferMatch {
  Any,        // any that match the regex
  ValidTheta, // any that match and have a valid theta value
  Strict      // only those that exactly match all parts of the regex
};

/** @class ReflRunsTabPresenter

ReflRunsTabPresenter is a presenter class for the Reflectometry Interface. It
handles any interface functionality and model manipulation.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflRunsTabPresenter
    : public IReflRunsTabPresenter,
      public MantidWidgets::DataProcessor::DataProcessorMainPresenter {
public:
  ReflRunsTabPresenter(IReflRunsTabView *mainView,
                       ProgressableView *progressView,
                       BatchPresenterFactory makeBatchPresenter,
                       double thetaTolerance,
                       std::vector<std::string> const &instruments,
                       int defaultInstrumentIndex,
                       boost::shared_ptr<IReflSearcher> searcher =
                           boost::shared_ptr<IReflSearcher>());

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
      std::string const &) override;
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
  BatchPresenterFactory m_makeBatchPresenter;
  /// The data processor presenters stored in a vector
  std::vector<std::unique_ptr<BatchPresenter>> m_tablePresenters;
  /// The main presenter
  IReflMainWindowPresenter *m_mainPresenter;
  /// The search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  /// The current search string used for autoreduction
  std::string m_autoSearchString;
  /// Legacy transfer method
  static const std::string LegacyTransferMethod;
  /// Measure transfer method
  static const std::string MeasureTransferMethod;
  /// Whether the instrument has been changed before a search was made with it
  bool m_instrumentChanged;
  double m_thetaTolerance;

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
  void changeInstrument();
  void changeGroup();
  void updateWidgetEnabledState() const;
  BatchPresenter *getTablePresenter(int group) const;
  /// Check that a given set of row indices are valid to transfer
  bool validateRowsToTransfer(const std::set<int> &rowsToTransfer);
  /// Get runs to transfer from row indices
  std::vector<SearchResult>
  getSearchResultRunDetails(const std::set<int> &rowsToTransfer);
  /// Get the data for a cell in the search results table as a string
  std::string searchModelData(const int row, const int column);
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H */
