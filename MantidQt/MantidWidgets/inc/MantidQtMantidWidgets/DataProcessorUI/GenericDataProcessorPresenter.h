#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessMap.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenterThread.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"

#include <QSet>
#include <queue>

#include <QObject>

using RowData = std::vector<std::string>;
using GroupData = std::map<int, RowData>;
using RowItem = std::pair<int, RowData>;
using RowQueue = std::queue<RowItem>;
using GroupQueue = std::queue<std::pair<int, RowQueue>>;

namespace MantidQt {
namespace MantidWidgets {
// Forward decs
class ProgressableView;
class DataProcessorView;
class DataProcessorTreeManager;
class GenericDataProcessorPresenterThread;

/** @class GenericDataProcessorPresenter

GenericDataProcessorPresenter is a presenter class for the Data Processor
Interface. It
handles any interface functionality and model manipulation.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS GenericDataProcessorPresenter
    : public QObject,
      public DataProcessorPresenter,
      public MantidQt::API::WorkspaceObserver {
  // Q_OBJECT for 'connect' with thread/worker
  Q_OBJECT

  friend class GenericDataProcessorPresenterRowReducerWorker;
  friend class GenericDataProcessorPresenterGroupReducerWorker;

public:
  // Constructor: pre-processing and post-processing
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
          preprocessMap,
      const DataProcessorProcessingAlgorithm &processor,
      const DataProcessorPostprocessingAlgorithm &postprocessor,
      const std::map<std::string, std::string> &postprocessMap =
          std::map<std::string, std::string>(),
      const std::string &loader = "Load");
  // Constructor: no pre-processing, post-processing
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const DataProcessorProcessingAlgorithm &processor,
      const DataProcessorPostprocessingAlgorithm &postprocessor);
  // Constructor: pre-processing, no post-processing
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
          preprocessMap,
      const DataProcessorProcessingAlgorithm &processor);
  // Constructor: no pre-processing, no post-processing
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const DataProcessorProcessingAlgorithm &processor);
  // Delegating constructor: pre-processing, no post-processing
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const DataProcessorPreprocessMap &preprocessMap,
      const DataProcessorProcessingAlgorithm &processor);
  // Delegating Constructor: pre-processing and post-processing
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const DataProcessorPreprocessMap &preprocessMap,
      const DataProcessorProcessingAlgorithm &processor,
      const DataProcessorPostprocessingAlgorithm &postprocessor);
  virtual ~GenericDataProcessorPresenter() override;
  void notify(DataProcessorPresenter::Flag flag) override;
  const std::map<std::string, QVariant> &options() const override;
  void setOptions(const std::map<std::string, QVariant> &options) override;
  void transfer(
      const std::vector<std::map<std::string, std::string>> &runs) override;
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &defaultInstrument) override;
  std::vector<std::unique_ptr<DataProcessorCommand>> publishCommands() override;
  void acceptViews(DataProcessorView *tableView,
                   ProgressableView *progressView) override;
  void accept(DataProcessorMainPresenter *mainPresenter) override;
  void setModel(std::string name) override;

  // The following methods are public only for testing purposes
  // Get the whitelist
  DataProcessorWhiteList getWhiteList() const { return m_whitelist; };
  // Get the name of the reduced workspace for a given row
  std::string getReducedWorkspaceName(const std::vector<std::string> &data,
                                      const std::string &prefix = "");
  // Get the name of a post-processed workspace
  std::string getPostprocessedWorkspaceName(const GroupData &groupData,
                                            const std::string &prefix = "");
  // Set the state of whether a new selection has been made
  void setNewSelectionState(bool newSelectionMade);

  ParentItems selectedParents() const override;
  ChildItems selectedChildren() const override;
  bool newSelectionMade() const override;
  bool askUserYesNo(const std::string &prompt,
                    const std::string &title) const override;
  void giveUserWarning(const std::string &prompt,
                       const std::string &title) const override;

protected:
  // The table view we're managing
  DataProcessorView *m_view;
  // The progress view
  ProgressableView *m_progressView;
  // A workspace receiver we want to notify
  DataProcessorMainPresenter *m_mainPresenter;
  // The tree manager, a proxy class to retrieve data from the model
  std::unique_ptr<DataProcessorTreeManager> m_manager;
  // Loader
  std::string m_loader;
  // The list of selected items to reduce
  TreeData m_selectedData;

  // Post-process some rows
  void postProcessGroup(const GroupData &data);
  // Reduce a row
  void reduceRow(RowData *data);
  // Finds a run in the AnalysisDataService
  std::string findRunInADS(const std::string &run, const std::string &prefix,
                           bool &runFound);
  // Sets whether to prompt user when getting selected runs
  void setPromptUser(bool allowPrompt);

  // Process selected rows
  virtual void process();
  // Plotting
  virtual void plotRow();
  virtual void plotGroup();
  void plotWorkspaces(const std::set<std::string> &workspaces);

protected slots:
  void reductionError(std::exception ex);
  void threadFinished(const int exitCode);

private:
  // the name of the workspace/table/model in the ADS, blank if unsaved
  std::string m_wsName;
  // The whitelist
  DataProcessorWhiteList m_whitelist;
  // The pre-processing instructions
  std::map<std::string, DataProcessorPreprocessingAlgorithm> m_preprocessMap;
  // The data processor algorithm
  DataProcessorProcessingAlgorithm m_processor;
  // Post-processing algorithm
  DataProcessorPostprocessingAlgorithm m_postprocessor;
  // Post-processing map
  std::map<std::string, std::string> m_postprocessMap;
  // The current queue of groups to be reduced
  GroupQueue m_gqueue;
  // The current group we are reducing row data for
  GroupData m_groupData;
  // The current row item being reduced
  RowItem m_rowItem;
  // The progress reporter
  ProgressPresenter *m_progressReporter;
  // A boolean indicating whether a post-processing algorithm has been defined
  bool m_postprocess;
  // The number of columns
  int m_columns;
  // A boolean indicating whether to prompt the user when getting selected runs
  bool m_promptUser;
  // stores whether or not the table has changed since it was last saved
  bool m_tableDirty;
  // stores whether a new table selection has been made before processing
  bool m_newSelection;
  // stores the user options for the presenter
  std::map<std::string, QVariant> m_options;
  // Thread to run reducer worker in
  std::unique_ptr<GenericDataProcessorPresenterThread> m_workerThread;
  // A boolean indicating whether or not data reduction has been paused
  mutable bool m_reductionPaused;
  // Enumeration of the reduction actions that can be taken
  enum class ReductionFlag { ReduceRowFlag, ReduceGroupFlag, StopReduceFlag };
  // A flag of the next action due to be carried out
  ReductionFlag m_nextActionFlag;
  // load a run into the ADS, or re-use one in the ADS if possible
  Mantid::API::Workspace_sptr getRun(const std::string &run,
                                     const std::string &instrument,
                                     const std::string &prefix);
  // Loads a run from disk
  std::string loadRun(const std::string &run, const std::string &instrument,
                      const std::string &prefix, const std::string &loader,
                      bool &runFound);
  // prepare a run or list of runs for processing
  Mantid::API::Workspace_sptr
  prepareRunWorkspace(const std::string &run,
                      const DataProcessorPreprocessingAlgorithm &alg,
                      const std::map<std::string, std::string> &optionsMap);
  // add row(s) to the model
  void appendRow();
  // add group(s) to the model
  void appendGroup();
  // delete row(s) from the model
  void deleteRow();
  // delete group(s) from the model
  void deleteGroup();
  // clear selected row(s) in the model
  void clearSelected();
  // copy selected rows to clipboard
  void copySelected();
  // copy selected rows to clipboard and then delete them
  void cutSelected();
  // paste clipboard into selected rows
  void pasteSelected();
  // group selected rows together
  void groupRows();
  // expand selection to group
  void expandSelection();
  // expand all groups
  void expandAll();
  // close all groups
  void collapseAll();
  // table io methods
  void newTable();
  void openTable();
  void saveTable();
  void saveTableAs();
  void importTable();
  void exportTable();

  // options
  void showOptionsDialog();
  void initOptions();

  // actions/commands
  void addCommands();

  // decide between processing next row or group
  void doNextAction();

  // process next row/group
  void nextRow();
  void nextGroup();

  // start thread for performing reduction on current row/group asynchronously
  virtual void startAsyncRowReduceThread(RowItem *rowItem, int groupIndex);
  virtual void startAsyncGroupReduceThread(GroupData &groupData,
                                           int groupIndex);

  // end reduction
  void endReduction();

  // pause/resume reduction
  void pause();
  void resume();

  // List of workspaces the user can open
  QSet<QString> m_workspaceList;

  void addHandle(const std::string &name,
                 Mantid::API::Workspace_sptr workspace) override;
  void postDeleteHandle(const std::string &name) override;
  void clearADSHandle() override;
  void renameHandle(const std::string &oldName,
                    const std::string &newName) override;
  void afterReplaceHandle(const std::string &name,
                          Mantid::API::Workspace_sptr workspace) override;
  void saveNotebook(
      const std::map<int, std::map<int, std::vector<std::string>>> &data);
  std::vector<std::unique_ptr<DataProcessorCommand>> getTableList();
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H*/
