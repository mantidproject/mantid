#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class GroupInfo;
class RowData;
using RowData_sptr = std::shared_ptr<RowData>;

/** QTwoLevelTreeModel : Provides a QAbstractItemModel for a
DataProcessorUI with post-processing defined. The first argument to the
constructor is a Mantid ITableWorkspace containing the values to use in the
reduction. Each row corresponds to an independent reduction and the first column
must contain the name of the group to which the corresponding row belongs. Rows
in the same group will be post-processed together. The second argument is a
WhiteList containing the header of the model, i.e. the name of the columns that
will be displayed in the tree. As the group is not shown as a column but as a
parent item, the table workspace must have one extra column with respect to the
number of items in the WhiteList.

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_COMMON QTwoLevelTreeModel : public AbstractTreeModel {
  Q_OBJECT
public:
  QTwoLevelTreeModel(Mantid::API::ITableWorkspace_sptr tableWorkspace,
                     const WhiteList &whitelist);
  ~QTwoLevelTreeModel() override;

  // Functions to read data from the model

  // Get data for a cell
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  // Get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  // Get row metadata
  RowData_sptr rowData(const QModelIndex &index) const override;
  // Row count
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  // Get the index for a given column, row and parent
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  // Get the 'processed' status of a row
  bool isProcessed(int position,
                   const QModelIndex &parent = QModelIndex()) const override;
  // Check whether reduction failed for a row/group
  bool
  reductionFailed(int position,
                  const QModelIndex &parent = QModelIndex()) const override;
  // Get the underlying data structure
  Mantid::API::ITableWorkspace_sptr getTableWorkspace() const;

  // Get the parent
  QModelIndex parent(const QModelIndex &index) const override;
  // Find or create a group
  int findOrAddGroup(const std::string &groupName);

  // Functions to edit model

  // Change or add data to the model
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  // Add new rows to the model
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  // Remove rows from the model
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  // Remove all rows from the model
  bool removeAll();
  // Set the 'processed' status of a row / group
  bool setProcessed(bool processed, int position,
                    const QModelIndex &parent = QModelIndex()) override;
  // Set the error message for a row / group
  bool setError(const std::string &error, int position,
                const QModelIndex &parent = QModelIndex()) override;
  // Insert rows
  bool insertRows(int position, int count, int parent);
  // Transfer rows into the table
  void transfer(const std::vector<std::map<QString, QString>> &runs) override;
private slots:
  void tableDataUpdated(const QModelIndex &, const QModelIndex &);

private:
  void updateGroupData(const int groupIdx, const int start, const int end);
  void updateAllGroupData();
  std::string cellValue(int groupIndex, int rowIndex, int columnIndex) const;
  bool runListsMatch(const std::string &newValue, const std::string &oldValue,
                     const bool exactMatch) const;
  bool rowMatches(int groupIndex, int rowIndex,
                  const std::map<QString, QString> &rowValues,
                  const bool exactMatch) const;
  boost::optional<int>
  findRowIndex(int group, const std::map<QString, QString> &rowValues) const;
  void insertRowWithValues(int groupIndex, int rowIndex,
                           const std::map<QString, QString> &rowValues);
  void insertRowAndGroupWithValues(const std::map<QString, QString> &rowValues);
  bool rowIsEmpty(int row, int parent) const;
  void setupModelData(Mantid::API::ITableWorkspace_sptr table);
  bool insertGroups(int position, int count);
  bool removeGroups(int position, int count);
  bool removeRows(int position, int count, int parent);
  // Check whether an index corresponds to a group or a row
  bool indexIsGroup(const QModelIndex &index) const;
  // Get data for a cell for particular roles
  QVariant getEditRole(const QModelIndex &index) const;
  QVariant getDisplayRole(const QModelIndex &index) const;
  QVariant getBackgroundRole(const QModelIndex &index) const;
  QVariant getToolTipRole(const QModelIndex &index) const;

  RowData_sptr rowData(int groupIndex, int rowIndex) const;

  /// List of all groups ordered by the group's position in the tree
  std::vector<GroupInfo> m_groups;
};

template <typename Action>
void forEachGroup(QTwoLevelTreeModel &model, Action act) {
  for (int group = 0; group < model.rowCount(); group++)
    act(group);
}

template <typename Action>
void forEachRow(QTwoLevelTreeModel &model, Action act) {
  forEachGroup(model, [&model, &act](int group) -> void {
    for (int row = 0; row < model.rowCount(model.index(group, 0)); row++)
      act(group, row);
  });
}

/// Typedef for a shared pointer to \c QTwoLevelTreeModel
using QTwoLevelTreeModel_sptr = boost::shared_ptr<QTwoLevelTreeModel>;
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace Mantid

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODEL_H_ */
