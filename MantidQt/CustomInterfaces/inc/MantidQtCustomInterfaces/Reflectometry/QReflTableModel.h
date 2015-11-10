#ifndef MANTID_CUSTOMINTERFACES_QREFLTABLEMODEL_H_
#define MANTID_CUSTOMINTERFACES_QREFLTABLEMODEL_H_


#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>


namespace MantidQt
{
  namespace CustomInterfaces
  {

    /** QReflTableModel : Provides a QAbstractTableModel for a Mantid ITableWorkspace.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTIDQT_CUSTOMINTERFACES_DLL QReflTableModel : public QAbstractTableModel
    {
      Q_OBJECT
    public:
      QReflTableModel(Mantid::API::ITableWorkspace_sptr tableWorkspace);
      virtual ~QReflTableModel();
      //emit a signal saying things have changed
      void update();
      //row and column counts
      int rowCount(const QModelIndex &parent = QModelIndex()) const;
      int columnCount(const QModelIndex &parent = QModelIndex()) const;
      //get data fro a cell
      QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
      //get header data for the table
      QVariant headerData(int section, Qt::Orientation orientation, int role) const;
      //get flags for a cell
      Qt::ItemFlags flags(const QModelIndex &index) const;
      //change or add data to the model
      bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
      //add new rows to the model
      bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
      //remove rows from the model
      bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
    private:

      //cache for a row's data
      mutable std::vector<QString> m_dataCache;
      //the index of the current row held in cache
      mutable int m_dataCachePeakIndex;

      //get a column's name
      QString findColumnName(const int colIndex) const;
      //update data cache if required
      void updateDataCache(const int row) const;
      //invalidate a row's data cache
      void invalidateDataCache(const int row) const;

      /// Collection of data for viewing.
      Mantid::API::ITableWorkspace_sptr m_tWS;

      /// Map of column indexes to names
      ReflTableSchema::ColumnIndexNameMap m_columnNameMap;
    };

    /// Typedef for a shared pointer to \c QReflTableModel
    typedef boost::shared_ptr<QReflTableModel> QReflTableModel_sptr;

  } // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_QREFLTABLEMODEL_H_ */
