#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include "boost/bind.hpp"
#include "boost/function.hpp"
#include "boost/tuple/tuple.hpp"
#include <map>
#include <vector>

// Forward declarations
namespace Mantid
{
  namespace API
  {
    class IPeaksWorkspace;
    class IPeak;
  }
}

namespace MantidQt
{
  namespace SliceViewer
  {
    /** @class QtWorkspaceMementoModel

    QAbstractTableModel for serving up PeaksWorkspaces.

    @author Owen Arnold
    @date 07/01/2013

    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class QPeaksTableModel : public QAbstractTableModel
    {
      Q_OBJECT
    public:
      QPeaksTableModel(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);
      void update();
      int rowCount(const QModelIndex &parent) const;
      int columnCount(const QModelIndex &parent) const;
      QVariant data(const QModelIndex &index, int role) const;
      QVariant headerData(int section, Qt::Orientation orientation, int role) const;
      Qt::ItemFlags flags(const QModelIndex &index) const;
      void sort (int column, Qt::SortOrder);
      int numCharacters(const int column) const;
      ~QPeaksTableModel();
     signals:
      void peaksSorted(const std::string&, const bool);
    private:

      typedef QString ColumnNameType;
      typedef QString ColumnValueType;
      typedef std::map<ColumnNameType, bool> ColumnNameSortableMap;
      typedef std::map<int, ColumnNameType> ColumnIndexNameMap;

      static const QString RUNNUMBER;
      static const QString DETID;
      static const QString H;
      static const QString K;
      static const QString L;
      static const QString DSPACING;
      static const QString INT;
      static const QString SIGMINT;
      static const QString QLAB;
      static const QString QSAMPLE;

      mutable std::vector<QString> m_dataCache;
      mutable int m_dataCachePeakIndex;

      QString findColumnName(const int colIndex) const;
      void updateDataCache(const Mantid::API::IPeak& peak, const int row) const;

      /// Collection of data for viewing.
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> m_peaksWS;

      /// Map of column indexes to names
      ColumnIndexNameMap m_columnNameMap;

      /// Map of column names to sortable flag.
      ColumnNameSortableMap m_sortableColumns;
    };
  }
}
