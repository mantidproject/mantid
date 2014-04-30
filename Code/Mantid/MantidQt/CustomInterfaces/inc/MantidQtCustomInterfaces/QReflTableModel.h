#ifndef MANTID_CUSTOMINTERFACES_QREFLTABLEMODEL_H_
#define MANTID_CUSTOMINTERFACES_QREFLTABLEMODEL_H_

#include "MantidAPI/ITableWorkspace.h"
#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>


namespace MantidQt
{
  namespace CustomInterfaces
  {

    /** QReflTableModel : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class QReflTableModel : public QAbstractTableModel
    {
      Q_OBJECT
    public:
      QReflTableModel(Mantid::API::ITableWorkspace_sptr tWS);
      virtual ~QReflTableModel();
      void update();
      int rowCount(const QModelIndex &parent) const;
      int columnCount(const QModelIndex &parent) const;
      QVariant data(const QModelIndex &index, int role) const;
      QVariant headerData(int section, Qt::Orientation orientation, int role) const;
      Qt::ItemFlags flags(const QModelIndex &index) const;
    private:

      typedef QString ColumnNameType;
      typedef QString ColumnValueType;
      typedef std::map<int, ColumnNameType> ColumnIndexNameMap;

    public:
      /// Label for run number column
      static const QString RUNS;
      /// Label for angle column
      static const QString ANGLE;
      /// Label for transmission column
      static const QString TRANSMISSION;
      /// Label for qmin column
      static const QString QMIN;
      /// Label for qmax column
      static const QString QMAX;
      /// Label for dq/q column
      static const QString DQQ;
      /// Label for group column
      static const QString SCALE;
      /// Label for group column
      static const QString GROUP;

    private:
      /// Index for run number column
      static const int COL_RUNS;
      /// Index for angle column
      static const int COL_ANGLE;
      /// Index for transmission column
      static const int COL_TRANSMISSION;
      /// Index for qmin column
      static const int COL_QMIN;
      /// Index for qmax column
      static const int COL_QMAX;
      /// Index for dq/q column
      static const int COL_DQQ;
      /// Index for group column
      static const int COL_SCALE;
      /// Index for group column
      static const int COL_GROUP;

      mutable std::vector<QString> m_dataCache;
      mutable int m_dataCachePeakIndex;

      QString findColumnName(const int colIndex) const;
      void updateDataCache(const int row) const;

      /// Collection of data for viewing.
      Mantid::API::ITableWorkspace_sptr m_tWS;

      /// Map of column indexes to names
      ColumnIndexNameMap m_columnNameMap;
    };


  } // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_QREFLTABLEMODEL_H_ */
