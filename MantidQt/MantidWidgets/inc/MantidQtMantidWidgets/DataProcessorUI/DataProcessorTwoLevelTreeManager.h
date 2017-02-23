#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGER_H

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"

namespace MantidQt {
namespace MantidWidgets {

class DataProcessorPresenter;
class DataProcessorWhiteList;
class QDataProcessorTwoLevelTreeModel;

/** @class DataProcessorTwoLevelTreeManager

DataProcessorTwoLevelTreeManager is a concrete implementation of a
DataProcessorTreeManager that handles a two-level tree view (which corresponds
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorTwoLevelTreeManager
    : public DataProcessorTreeManager {
public:
  /// Constructor
  DataProcessorTwoLevelTreeManager(DataProcessorPresenter *presenter,
                                   Mantid::API::ITableWorkspace_sptr table,
                                   const DataProcessorWhiteList &whitelist);
  /// Constructor (no table ws given)
  DataProcessorTwoLevelTreeManager(DataProcessorPresenter *presenter,
                                   const DataProcessorWhiteList &whitelist);
  /// Destructor
  ~DataProcessorTwoLevelTreeManager() override;

  /// Publish commands
  std::vector<std::unique_ptr<DataProcessorCommand>> publishCommands() override;
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
  std::string copySelected() override;
  /// Paste selected
  void pasteSelected(const std::string &text) override;
  /// Blank table
  void newTable(const DataProcessorWhiteList &whitelist) override;
  /// New table
  void newTable(Mantid::API::ITableWorkspace_sptr table,
                const DataProcessorWhiteList &whitelist) override;

  /// Return selected data
  TreeData selectedData(bool prompt) override;
  /// Transfer new data to model
  void transfer(const std::vector<std::map<std::string, std::string>> &runs,
                const DataProcessorWhiteList &whitelist) override;
  /// Update row with new data
  void update(int parent, int child,
              const std::vector<std::string> &data) override;

  /// Validate a table workspace
  bool isValidModel(Mantid::API::Workspace_sptr ws,
                    size_t whitelistColumns) const override;

  /// Return the model
  boost::shared_ptr<QAbstractItemModel> getModel() override;
  /// Return the table workspace
  Mantid::API::ITableWorkspace_sptr getTableWorkspace() override;

private:
  /// The DataProcessor presenter
  DataProcessorPresenter *m_presenter;
  /// The model
  boost::shared_ptr<QDataProcessorTwoLevelTreeModel> m_model;

  /// Insert a row in the model
  void insertRow(int groupIndex, int rowIndex);
  /// Insert a group in the model
  void insertGroup(int groupIndex);
  /// Get the number of rows in a group
  int numRowsInGroup(int groupId) const;
  /// Create a default table workspace
  Mantid::API::ITableWorkspace_sptr
  createDefaultWorkspace(const DataProcessorWhiteList &whitelist);
  /// Validate a table workspace
  void validateModel(Mantid::API::ITableWorkspace_sptr ws,
                     size_t whitelistColumns) const;
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGER_H*/
