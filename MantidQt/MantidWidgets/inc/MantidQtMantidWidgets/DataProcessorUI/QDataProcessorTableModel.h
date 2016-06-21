#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTABLEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTABLEMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** QDataProcessorTableModel : Provides a QAbstractTableModel for a Mantid
ITableWorkspace.

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS QDataProcessorTableModel
    : public QAbstractTableModel {
  Q_OBJECT
public:
  QDataProcessorTableModel(Mantid::API::ITableWorkspace_sptr tableWorkspace,
                           const DataProcessorWhiteList &whitelist);
  ~QDataProcessorTableModel() override;
  // emit a signal saying things have changed
  void update();
  // row and column counts
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  // get data fro a cell
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  // get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  // get flags for a cell
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  // change or add data to the model
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  // add new rows to the model
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  // remove rows from the model
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;

private:
  // cache for a row's data
  mutable std::vector<QString> m_dataCache;
  // the index of the current row held in cache
  mutable int m_dataCachePeakIndex;

  // get a column's name
  QString findColumnName(const int colIndex) const;
  // update data cache if required
  void updateDataCache(const int row) const;
  // invalidate a row's data cache
  void invalidateDataCache(const int row) const;

  /// Collection of data for viewing.
  Mantid::API::ITableWorkspace_sptr m_tWS;

  /// Map of column indexes to names and viceversa
  DataProcessorWhiteList m_whitelist;
};

/// Typedef for a shared pointer to \c QDataProcessorTableModel
typedef boost::shared_ptr<QDataProcessorTableModel>
    QDataProcessorTableModel_sptr;

} // namespace MantidWidgets
} // namespace Mantid

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORTABLEMODEL_H_ */
