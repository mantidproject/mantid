#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtMantidWidgets/DataProcessorUI/AbstractDataProcessorTreeModel.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** QDataProcessorTwoLevelTreeModel : Provides a QAbstractItemModel for a
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS QDataProcessorTwoLevelTreeModel
    : public AbstractDataProcessorTreeModel {
  Q_OBJECT
public:
  QDataProcessorTwoLevelTreeModel(
      Mantid::API::ITableWorkspace_sptr tableWorkspace,
      const DataProcessorWhiteList &whitelist);
  ~QDataProcessorTwoLevelTreeModel() override;

  // Functions to read data from the model

  // Get data for a cell
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  // Get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  // Row count
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  // Get the index for a given column, row and parent
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  // Get the 'processed' status of a row
  bool isProcessed(int position,
                   const QModelIndex &parent = QModelIndex()) const override;
  // Get the underlying data structure
  Mantid::API::ITableWorkspace_sptr getTableWorkspace() const;

  // Get the parent
  QModelIndex parent(const QModelIndex &index) const override;

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
  // Set the 'processed' status of a row / group
  bool setProcessed(bool processed, int position,
                    const QModelIndex &parent = QModelIndex()) override;

private:
  void setupModelData(Mantid::API::ITableWorkspace_sptr table);
  bool insertGroups(int position, int count);
  bool insertRows(int position, int count, int parent);
  bool removeGroups(int position, int count);
  bool removeRows(int position, int count, int parent);

  /// Vector containing group names and process status
  std::vector<std::pair<std::string, bool>> m_groupName;
  /// Vector containing the (absolute) row indices for a given group and process
  /// status
  std::vector<std::vector<std::pair<int, bool>>> m_rowsOfGroup;
};

/// Typedef for a shared pointer to \c QDataProcessorTwoLevelTreeModel
typedef boost::shared_ptr<QDataProcessorTwoLevelTreeModel>
    QDataProcessorTwoLevelTreeModel_sptr;

} // namespace MantidWidgets
} // namespace Mantid

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODEL_H_ */
