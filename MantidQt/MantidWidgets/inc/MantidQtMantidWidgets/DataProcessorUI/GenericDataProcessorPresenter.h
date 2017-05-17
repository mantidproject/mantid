#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"

#include <set>

using GroupData = std::map<int, std::vector<std::string>>;

namespace MantidQt {
namespace MantidWidgets {
// Forward decs
class ProgressableView;
class DataProcessorView;
class DataProcessorTreeManager;

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
    : public DataProcessorPresenter,
      public MantidQt::API::WorkspaceObserver {
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

  ParentItems selectedParents() const override;
  ChildItems selectedChildren() const override;
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

  // Post-process some rows
  void postProcessGroup(const GroupData &data);
  // Reduce a row
  std::vector<std::string> reduceRow(const std::vector<std::string> &data);
  // Finds a run in the AnalysisDataService
  std::string findRunInADS(const std::string &run, const std::string &prefix,
                           bool &runFound);

  // Process selected rows
  virtual void process();
  // Plotting
  virtual void plotRow();
  virtual void plotGroup();
  void plotWorkspaces(const std::set<std::string> &workspaces);

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
  // A boolean indicating whether a post-processing algorithm has been defined
  bool m_postprocess;
  // The number of columns
  int m_columns;
  // stores whether or not the table has changed since it was last saved
  bool m_tableDirty;
  // stores the user options for the presenter
  std::map<std::string, QVariant> m_options;
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

  // List of workspaces the user can open
  std::set<std::string> m_workspaceList;

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
