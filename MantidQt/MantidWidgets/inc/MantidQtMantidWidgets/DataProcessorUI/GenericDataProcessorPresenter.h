#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithm.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTableModel.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"

namespace MantidQt {
namespace MantidWidgets {
// Forward decs
class ProgressableView;
class DataProcessorView;
class DataProcessorCommand;

/** @class GenericDataProcessorPresenter

GenericDataProcessorPresenter is a presenter class for the Data Processor
Interface. It
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS GenericDataProcessorPresenter
    : public DataProcessorPresenter,
      public MantidQt::API::WorkspaceObserver {
public:
  GenericDataProcessorPresenter(
      const DataProcessorWhiteList &whitelist,
      const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
          preprocessMap,
      const DataProcessorProcessingAlgorithm &processor,
      const DataProcessorPostprocessingAlgorithm &postprocessor);
  ~GenericDataProcessorPresenter() override;
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
  void setModel(std::string name) override;

  // Only for testing purposes
  DataProcessorWhiteList getWhiteList() const { return m_whitelist; };

protected:
  // the workspace the model is currently representing
  Mantid::API::ITableWorkspace_sptr m_ws;
  // the model
  QDataProcessorTableModel_sptr m_model;
  // the name of the workspace/table/model in the ADS, blank if unsaved
  std::string m_wsName;
  // the table view we're managing
  DataProcessorView *m_view;
  // The progress view
  ProgressableView *m_progressView;
  // The whitelist
  DataProcessorWhiteList m_whitelist;
  // The pre-processing instructions
  std::map<std::string, DataProcessorPreprocessingAlgorithm> m_preprocessMap;
  // The data processor algorithm
  DataProcessorProcessingAlgorithm m_processor;
  // Post-processing algorithm
  DataProcessorPostprocessingAlgorithm m_postprocessor;
  // The number of columns
  int m_columns;
  // A workspace receiver we want to notify
  WorkspaceReceiver *m_workspaceReceiver;
  // stores whether or not the table has changed since it was last saved
  bool m_tableDirty;
  // stores the user options for the presenter
  std::map<std::string, QVariant> m_options;
  // Post-process some rows
  void postProcessRows(std::set<int> rows);
  // process selected rows
  void process();
  // process groups of rows
  bool processGroups(std::map<int, std::set<int>> groups, std::set<int> rows);
  // Reduce a row
  void reduceRow(int rowNo);
  // prepare a run or list of runs for processing
  Mantid::API::Workspace_sptr
  prepareRunWorkspace(const std::string &run,
                      const DataProcessorPreprocessingAlgorithm &alg,
                      const std::map<std::string, std::string> &optionsMap);
  // Get the workspace name for a given row
  std::string getWorkspaceName(int row, bool prefix = true);
  // load a run into the ADS, or re-use one in the ADS if possible
  Mantid::API::Workspace_sptr loadRun(const std::string &run,
                                      const std::string &instrument,
                                      const std::string &prefix); // change
  // get an unused group id
  int getUnusedGroup(std::set<int> ignoredRows = std::set<int>()) const;
  // get the number of rows in a group
  size_t numRowsInGroup(int groupId) const;
  // Validate rows
  bool rowsValid(std::set<int> rows);
  // Validate a row
  void validateRow(int rowNo) const;
  // insert a row in the model before the given index
  void insertRow(int index);
  // add row(s) to the model
  void appendRow();
  void prependRow();
  // delete row(s) from the model
  void deleteRow();
  // find a blank row
  int getBlankRow();
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
  // table io methods
  void newTable();
  void openTable();
  void saveTable();
  void saveTableAs();
  void importTable();
  void exportTable();
  // plotting
  void plotRow();
  void plotGroup();
  // options
  void showOptionsDialog();
  void initOptions();
  // algorithms' hints
  void createProcessLayout();

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
  void saveNotebook(std::map<int, std::set<int>> groups, std::set<int> rows);
  void accept(WorkspaceReceiver *workspaceReceiver) override;
  std::vector<std::unique_ptr<DataProcessorCommand>> getTableList();

  void validateModel(Mantid::API::ITableWorkspace_sptr model);
  bool isValidModel(Mantid::API::Workspace_sptr model);
  Mantid::API::ITableWorkspace_sptr createWorkspace();
  Mantid::API::ITableWorkspace_sptr createDefaultWorkspace();
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_H*/
