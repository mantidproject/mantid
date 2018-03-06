#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGER_H

#include "MantidQtWidgets/Common/DataProcessorUI/TreeManager.h"
#include "MantidQtWidgets/Common/DllOption.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

using ChildItems = std::map<int, std::set<int>>;

class DataProcessorPresenter;
class WhiteList;
class QTwoLevelTreeModel;

/** @class TwoLevelTreeManager

TwoLevelTreeManager is a concrete implementation of a
TreeManager that handles a two-level tree view (which corresponds
to a DataProcessorUI with a post-processing algorithm defined).

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
class EXPORT_OPT_MANTIDQT_COMMON TwoLevelTreeManager : public TreeManager {
public:
  /// Constructor
  TwoLevelTreeManager(DataProcessorPresenter *presenter,
                      Mantid::API::ITableWorkspace_sptr table,
                      const WhiteList &whitelist);
  /// Constructor (no table ws given)
  TwoLevelTreeManager(DataProcessorPresenter *presenter,
                      const WhiteList &whitelist);
  /// Destructor
  ~TwoLevelTreeManager() override;

  /// Publish commands
  std::vector<std::unique_ptr<Command>> publishCommands() override;
  /// Append a row
  void appendRow() override;
  /// Append a group to the model
  void appendGroup() override;
  /// Delete a row
  void deleteRow() override;
  /// Delete a group
  void deleteGroup() override;
  /// Group rows
  void groupRows() override;
  /// Expand selection
  std::set<int> expandSelection() override;
  /// Clear selected
  void clearSelected() override;
  /// Copy selected
  QString copySelected() override;
  /// Paste selected
  void pasteSelected(const QString &text) override;
  /// Blank table
  void newTable(const WhiteList &whitelist) override;
  /// New table
  void newTable(Mantid::API::ITableWorkspace_sptr table,
                const WhiteList &whitelist) override;

  /// Return selected data
  TreeData selectedData(bool prompt) override;
  /// Transfer new data to model
  void transfer(const std::vector<std::map<QString, QString>> &runs) override;
  /// Update row with new data
  void update(int parent, int child, const QStringList &data) override;
  /// Get the number of rows of a given parent
  int rowCount() const override;
  int rowCount(int parent) const override;
  void setCell(int row, int column, int parentRow, int parentColumn,
               const std::string &value) override;
  std::string getCell(int row, int column, int parentRow,
                      int parentColumn) const override;
  int getNumberOfRows() override;
  /// Get the 'processed' status of a data item
  bool isProcessed(int position) const override;
  bool isProcessed(int position, int parent) const override;
  /// Set the 'process' status of a data item
  void setProcessed(bool processed, int position) override;
  void setProcessed(bool processed, int position, int parent) override;
  /// Check whether reduction failed for an item
  bool reductionFailed(int position) const override;
  bool reductionFailed(int position, int parent) const override;
  /// Set the error message of a data item
  void setError(const std::string &error, int position) override;
  void setError(const std::string &error, int position, int parent) override;
  void invalidateAllProcessed() override;

  /// Validate a table workspace
  bool isValidModel(Mantid::API::Workspace_sptr ws,
                    size_t whitelistColumns) const override;

  /// Return the model
  boost::shared_ptr<AbstractTreeModel> getModel() override;
  /// Return the table workspace
  Mantid::API::ITableWorkspace_sptr getTableWorkspace() override;

  bool isMultiLevel() const override;

private:
  /// The DataProcessor presenter
  DataProcessorPresenter *m_presenter;
  /// The model
  boost::shared_ptr<QTwoLevelTreeModel> m_model;

  /// Check whether a row matches the given row in the model
  bool rowMatches(int groupIndex, int rowIndex,
                  const std::map<QString, QString> &rowValues,
                  const WhiteList &whitelist) const;
  /// Check whether a row with the given values exists
  bool rowExists(const std::map<QString, QString> &rowValues,
                 const WhiteList &whitelist) const;
  /// Insert an empty row in the model
  void insertRow(int groupIndex, int rowIndex);
  /// Insert a group in the model
  void insertGroup(int groupIndex);
  /// Get the number of rows in a group
  int numRowsInGroup(int groupId) const;
  /// Create a default table workspace
  Mantid::API::ITableWorkspace_sptr
  createDefaultWorkspace(const WhiteList &whitelist);
  /// Validate a table workspace
  void validateModel(Mantid::API::ITableWorkspace_sptr ws,
                     size_t whitelistColumns) const;
  TreeData constructTreeData(ChildItems rows);
};
}
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGER_H*/
