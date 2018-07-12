#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class WhiteList;

/** @class TreeManager

TreeManager is an abstract base class defining some methods meant
to be used by the Generic Data Processor presenter, which will delegate some
functionality to concrete tree manager implementations, depending on whether or
not a post-processing algorithm has been defined.

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

class TreeManager {
public:
  virtual ~TreeManager(){};

  /// Actions/commands
  virtual bool isMultiLevel() const = 0;
  /// Publish actions/commands
  virtual std::vector<std::unique_ptr<Command>> publishCommands() = 0;
  /// Append a row
  virtual void appendRow() = 0;
  /// Append a group
  virtual void appendGroup() = 0;
  /// Delete a row
  virtual void deleteRow() = 0;
  /// Delete a group
  virtual void deleteGroup() = 0;
  /// Delete all rows and groups
  virtual void deleteAll() = 0;
  /// Group rows
  virtual void groupRows() = 0;
  /// Expand selection
  virtual std::set<int> expandSelection() = 0;
  /// Clear selected
  virtual void clearSelected() = 0;
  /// Copy selected
  virtual QString copySelected() = 0;
  /// Paste selected
  virtual void pasteSelected(const QString &text) = 0;
  /// Blank table
  virtual void newTable(const WhiteList &whitelist) = 0;
  /// Blank table
  virtual void newTable(Mantid::API::ITableWorkspace_sptr table,
                        const WhiteList &whitelist) = 0;

  /// Read/write data

  /// Return selected data
  virtual TreeData selectedData(bool prompt = false) = 0;
  /// Return all data
  virtual TreeData allData(bool prompt = false) = 0;
  /// Transfer new data to model
  virtual void
  transfer(const std::vector<std::map<QString, QString>> &runs) = 0;
  /// Update row with new data
  virtual void update(int parent, int child, const QStringList &data) = 0;
  /// Get the number of rows of a given parent
  virtual int rowCount() const = 0;
  virtual int rowCount(int parent) const = 0;
  /// Get the 'processed' status of a data item
  virtual bool isProcessed(int position) const = 0;
  virtual bool isProcessed(int position, int parent) const = 0;
  /// Set the 'processed' status of a data item
  virtual void setProcessed(bool processed, int position) = 0;
  virtual void setProcessed(bool processed, int position, int parent) = 0;
  /// Check whether reduction failed for a data item
  virtual bool reductionFailed(int position) const = 0;
  virtual bool reductionFailed(int position, int parent) const = 0;
  /// Set the error message for a data item
  virtual void setError(const std::string &error, int position) = 0;
  virtual void setError(const std::string &error, int position, int parent) = 0;
  /// Reset the processed/error state of all items
  virtual void invalidateAllProcessed() = 0;
  /// Access cells
  virtual void setCell(int row, int column, int parentRow, int parentColumn,
                       const std::string &value) = 0;
  virtual std::string getCell(int row, int column, int parentRow,
                              int parentColumn) const = 0;
  virtual int getNumberOfRows() = 0;
  /// Validate a table workspace
  virtual bool isValidModel(Mantid::API::Workspace_sptr ws,
                            size_t whitelistColumns) const = 0;

  /// Return member variables

  /// Return the model
  virtual boost::shared_ptr<AbstractTreeModel> getModel() = 0;
  /// Return the table ws
  virtual Mantid::API::ITableWorkspace_sptr getTableWorkspace() = 0;

protected:
  /// Add a command to the list of available commands
  void addCommand(std::vector<std::unique_ptr<Command>> &commands,
                  std::unique_ptr<Command> command) {
    commands.push_back(std::move(command));
  }
};
}
}
}
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H */
