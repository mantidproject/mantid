#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSearcher.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTransferStrategy.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"

#include <Poco/AutoPtr.h>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
// Forward decs
class ProgressableView;

/** @class ReflMainViewPresenter
ReflMainViewPresenter is a presenter class for teh Reflectometry Interface. It
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
class MANTIDQT_CUSTOMINTERFACES_DLL ReflMainViewPresenter
    : public IReflPresenter,
      public MantidQt::API::WorkspaceObserver {
public:
  ReflMainViewPresenter(ReflMainView *mainView, ProgressableView *progressView,
                        boost::shared_ptr<IReflSearcher> searcher =
                            boost::shared_ptr<IReflSearcher>());
  virtual ~ReflMainViewPresenter();
  virtual void notify(IReflPresenter::Flag flag);
  virtual const std::map<std::string, QVariant> &options() const;
  virtual void setOptions(const std::map<std::string, QVariant> &options);

protected:
  // the workspace the model is currently representing
  Mantid::API::ITableWorkspace_sptr m_ws;
  // the models
  QReflTableModel_sptr m_model;
  ReflSearchModel_sptr m_searchModel;
  // the name of the workspace/table/model in the ADS, blank if unsaved
  std::string m_wsName;
  // the main view we're managing
  ReflMainView *m_view;
  // The progress view
  ProgressableView *m_progressView;
  // stores whether or not the table has changed since it was last saved
  bool m_tableDirty;
  // stores the user options for the presenter
  std::map<std::string, QVariant> m_options;
  // the search implementation
  boost::shared_ptr<IReflSearcher> m_searcher;
  // process selected rows
  void process();
  // process groups of rows
  bool processGroups(std::map<int, std::set<int>> groups, std::set<int> rows);
  // Reduce a row
  void reduceRow(int rowNo);
  // prepare a run or list of runs for processing
  Mantid::API::Workspace_sptr prepareRunWorkspace(const std::string &run);
  // load a run into the ADS, or re-use one in the ADS if possible
  Mantid::API::Workspace_sptr loadRun(const std::string &run,
                                      const std::string &instrument); // change
  // get the run number of a TOF workspace
  std::string getRunNumber(const Mantid::API::Workspace_sptr &ws);
  // get an unused group id
  int getUnusedGroup(std::set<int> ignoredRows = std::set<int>()) const;
  // make a transmission workspace
  Mantid::API::Workspace_sptr makeTransWS(const std::string &transString);
  // Validate rows
  bool rowsValid(std::set<int> rows);
  // Validate a row
  void validateRow(int rowNo) const;
  // Autofill a row with sensible values
  void autofillRow(int rowNo);
  // calculates qmin and qmax
  std::vector<double> calcQRange(Mantid::API::Workspace_sptr ws, double theta);
  // get the number of rows in a group
  size_t numRowsInGroup(int groupId) const;
  // Stitch some rows
  void stitchRows(std::set<int> rows);
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
  // searching
  void search();
  void populateSearch(Mantid::API::IAlgorithm_sptr searchAlg);
  void transfer();
  // plotting
  void plotRow();
  void plotGroup();
  // options
  void showOptionsDialog();
  void initOptions();

  // List of workspaces the user can open
  std::set<std::string> m_workspaceList;

  void addHandle(const std::string &name,
                 Mantid::API::Workspace_sptr workspace);
  void postDeleteHandle(const std::string &name);
  void clearADSHandle();
  void renameHandle(const std::string &oldName, const std::string &newName);
  void afterReplaceHandle(const std::string &name,
                          Mantid::API::Workspace_sptr workspace);
  void saveNotebook(std::map<int, std::set<int>> groups, std::set<int> rows);

private:
  static const std::string LegacyTransferMethod;
  static const std::string MeasureTransferMethod;

  std::unique_ptr<ReflTransferStrategy> getTransferStrategy();
};
}
}
#endif