#ifndef MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H

#include "DllConfig.h"
#include "IReflRunsTabPresenter.h"
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
  void confirmReductionFinished(int group) override;
  void confirmReductionPaused(int group) override;
  void confirmReductionResumed(int group) override;
  void settingsChanged(int group) override;
  void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &) override;
  void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceNames) override;
  bool autoreductionRunning() const override;
  bool autoreductionRunning(int group) const override;

private:
  /// The search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
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
  /// The current transfer method
  std::string m_currentTransferMethod;
  /// Legacy transfer method
  static const std::string LegacyTransferMethod;
  /// Measure transfer method
  static const std::string MeasureTransferMethod;
  /// Whether the instrument has been changed before a search was made with it
  bool m_instrumentChanged;
  /// Information about the autoreduction process
  ReflAutoreduction m_autoreduction;

  /// searching
  void search();
  void icatSearchComplete();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  /// autoreduction
  void startNewAutoreduction();
  void startAutoreduction();
  void pauseAutoreduction();
  void runAutoreduction();
  ProgressPresenter setupProgressBar(const std::set<int> &rowsToTransfer);
  void transfer(const std::set<int> &rowsToTransfer, int group,
                const TransferMatch matchType = TransferMatch::Any);
  void pushCommands(int group);
  /// transfer strategy
  std::unique_ptr<ReflTransferStrategy> getTransferStrategy();
  /// change the instrument
  void changeInstrument();
  /// enable/disable widgets on the view
  void updateWidgetEnabledState(const bool isProcessing) const;
  /// Determine whether to start a new autoreduction
  bool requireNewAutoreduction() const;
  /// Check that a given set of row indices are valid to transfer
  bool validateRowsToTransfer(const std::set<int> &rowsToTransfer);
  /// Get runs to transfer from row indices
  SearchResultMap
  getSearchResultRunDetails(const std::set<int> &rowsToTransfer);
  /// Deal with rows that are invalid for transfer
  void handleInvalidRunsForTransfer(
      const std::set<int> &rowsToTransfer,
      const std::vector<TransferResults::COLUMN_MAP_TYPE> &invalidRuns);
  /// Get the data for a cell in the search results table as a string
  std::string searchModelData(const int row, const int column);
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLRUNSTABPRESENTER_H */
