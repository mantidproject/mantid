#ifndef MANTIDQTMANTIDWIDGETS_ABSTRACTDATAPROCESSORTREEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_ABSTRACTDATAPROCESSORTREEMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <QAbstractItemModel>
#include <QColor>

namespace MantidQt {
namespace MantidWidgets {

/** AbstractDataProcessorTreeModel is a base class for several tree model
implementations for processing table data. Full function implementation is
provided for functions common to all data processing tree models, while
these subclasses are expected to provide implementation for the remaining
virtual functions defined here and in QAbstractItemModel.

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS AbstractDataProcessorTreeModel
    : public QAbstractItemModel {
  Q_OBJECT
public:
  AbstractDataProcessorTreeModel(
      Mantid::API::ITableWorkspace_sptr tableWorkspace,
      const DataProcessorWhiteList &whitelist);
  ~AbstractDataProcessorTreeModel() override;

  // Functions to read data from the model

  // Column count
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  // Get flags for a cell
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  // Get the 'processed' status of a data item
  virtual bool isProcessed(int position,
                           const QModelIndex &parent = QModelIndex()) const = 0;
  // Set the 'processed' status of a data item
  virtual bool setProcessed(bool processed, int position,
                            const QModelIndex &parent = QModelIndex()) = 0;

protected:
  /// Collection of data for viewing.
  Mantid::API::ITableWorkspace_sptr m_tWS;
  /// Map of column indexes to names and viceversa
  DataProcessorWhiteList m_whitelist;
};
} // namespace MantidWidgets
} // namespace Mantid

#endif /* MANTIDQTMANTIDWIDGETS_ABSTRACTDATAPROCESSORTREEMODEL_H_ */
