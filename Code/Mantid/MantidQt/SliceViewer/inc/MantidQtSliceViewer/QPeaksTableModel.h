#include <QAbstractTableModel>
#include "DllOption.h"
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

    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class EXPORT_OPT_MANTIDQT_SLICEVIEWER QPeaksTableModel : public QAbstractTableModel
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
      std::vector<int> defaultHideCols();
      ~QPeaksTableModel();
      void setPeaksWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);
     signals:
      void peaksSorted(const std::string&, const bool);
    private:

      typedef QString ColumnNameType;
      typedef QString ColumnValueType;
      typedef std::map<ColumnNameType, bool> ColumnNameSortableMap;
      typedef std::map<int, ColumnNameType> ColumnIndexNameMap;

    public:
      /// Label for run number column
      static const QString RUNNUMBER;
      /// Label for detector id column
      static const QString DETID;
      /// Label for h column
      static const QString H;
      /// Label for k column
      static const QString K;
      /// Label for l column
      static const QString L;
      /// Label for wavelength column
      static const QString WAVELENGTH;
      /// Label for change in energy column
      static const QString ENERGY;
      /// Label for initial energy column
      static const QString INITIAL_ENERGY;
      /// Label for final energy column
      static const QString FINAL_ENERGY;
      /// Label for time-of-flight column
      static const QString TOF;
      /// Label for d-spacing column
      static const QString DSPACING;
      /// Label for integrated intensity column
      static const QString INT;
      /// Label for uncertainty in integrated intensity column
      static const QString SIGMINT;
      /// Label for ratio of intensity/uncertainty
      static const QString INT_SIGINT;
      /// Label for bin count column
      static const QString BINCOUNT;
      /// Label for bank name column
      static const QString BANKNAME;
      /// Label for detector row column
      static const QString ROW;
      /// Label for detector column column
      static const QString COL;
      /// Label for Q-vector in the lab frame column
      static const QString QLAB;
      /// Label for Q-vector in the sample column
      static const QString QSAMPLE;

    private:
      /// Index for run number column
      static const int COL_RUNNUMBER;
      /// Index for detector id column
      static const int COL_DETID;
      /// Index for h column
      static const int COL_H;
      /// Index for k column
      static const int COL_K;
      /// Index for l column
      static const int COL_L;
      /// Index for wavelength column
      static const int COL_WAVELENGTH;
      /// Index for change in energy column
      static const int COL_ENERGY;
      /// Index for initial energy column
      static const int COL_INITIAL_ENERGY;
      /// Index for final energy column
      static const int COL_FINAL_ENERGY;
      /// Index for time-of-flight column
      static const int COL_TOF;
      /// Index for d-spacing column
      static const int COL_DSPACING;
      /// Index for integrated intensity column
      static const int COL_INT;
      /// Index for uncertainty in integrated intensity column
      static const int COL_SIGMINT;
      /// Label for ratio of intensity/uncertainty
      static const int COL_INT_SIGINT;
      /// Index for bin count column
      static const int COL_BINCOUNT;
      /// Index for bank name column
      static const int COL_BANKNAME;
      /// Index for detector row column
      static const int COL_ROW;
      /// Index for detector column column
      static const int COL_COL;
      /// Index for Q-vector in the lab frame column
      static const int COL_QLAB;
      /// Index for Q-vector in the sample column
      static const int COL_QSAMPLE;

      /// The number of digits past the decimal to display in the table
      int m_hklPrec;

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
