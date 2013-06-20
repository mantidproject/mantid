#include <MantidQtSliceViewer/QPeaksTableModel.h>
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include <MantidKernel/ConfigService.h>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace boost;

namespace MantidQt
{
  namespace SliceViewer
  {
    const QString QPeaksTableModel::RUNNUMBER = "Run";
    const QString QPeaksTableModel::DETID = "DetID";
    const QString QPeaksTableModel::H = "h";
    const QString QPeaksTableModel::K= "k";
    const QString QPeaksTableModel::L = "l";
    const QString QPeaksTableModel::DSPACING = "DSpacing";
    const QString QPeaksTableModel::INT = "Intens";
    const QString QPeaksTableModel::SIGMINT = "SigInt";
    const QString QPeaksTableModel::QLAB = "QLab";
    const QString QPeaksTableModel::QSAMPLE = "QSample";

    void QPeaksTableModel::updateDataCache(const Mantid::API::IPeak& peak, const int row) const
    {
      // if the index is what is already cached just return
      if (row == m_dataCachePeakIndex)
        return;

      // generate the cache
      m_dataCache.clear();
      m_dataCache.push_back(QString::number(peak.getRunNumber()));
      m_dataCache.push_back(QString::number(peak.getDetectorID()));
      m_dataCache.push_back(QString::number(peak.getH(), 'f', m_hklPrec));
      m_dataCache.push_back(QString::number(peak.getK(), 'f', m_hklPrec));
      m_dataCache.push_back(QString::number(peak.getL(), 'f', m_hklPrec));
      m_dataCache.push_back(QString::number(peak.getDSpacing(), 'f', 4));
      m_dataCache.push_back(QString::number(peak.getIntensity()));
      m_dataCache.push_back(QString::number(peak.getSigmaIntensity()));

      const QString COMMA(",");

      const Mantid::Kernel::V3D qlab = peak.getQLabFrame();
      m_dataCache.push_back(QString::number(qlab.X(), 'f', 4) + COMMA + QString::number(qlab.Y(), 'f', 4) + COMMA + QString::number(qlab.Z(), 'f', 4));

      const Mantid::Kernel::V3D qsample = peak.getQSampleFrame();
      m_dataCache.push_back(QString::number(qsample.X(), 'f', 4) + COMMA + QString::number(qsample.Y(), 'f', 4) + COMMA + QString::number(qsample.Z(), 'f', 4));
    }

    /**
     * @param column The column to get the number of characters
     * estimated for.
     *
     * @return The number of characters estimated for the column. If
     * it cannot be determined this returns 10.
     */
    int QPeaksTableModel::numCharacters(const int column) const
    {
      if (column == 0) // RUNNUMBER
        return 5;
      else if (column == 1) // DETID
        return 7;
      else if (column == 2) // H
        return 3+m_hklPrec;
      else if (column == 3) // K
        return 3+m_hklPrec;
      else if (column == 4) // L
        return 3+m_hklPrec;
      else if (column == 5) // DSPACING
        return 6;
      else if (column == 6) // INT
        return 5;
      else if (column == 7) // SIGMINT
          return 5;
      else if (column == 8) // QLAB
          return 3*6;
      else if (column == 9) // QSAMPLE
          return 3*6;
      else
        return 3;
    }

    /**
    Constructor
    @param peaksWS : Workspace model.
    */
    QPeaksTableModel::QPeaksTableModel(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) :
        m_dataCachePeakIndex(-1),
        m_peaksWS(peaksWS)
    {
      int index = 0;
      m_columnNameMap.insert(std::make_pair(index++, RUNNUMBER));
      m_columnNameMap.insert(std::make_pair(index++, DETID));
      m_columnNameMap.insert(std::make_pair(index++, H));
      m_columnNameMap.insert(std::make_pair(index++, K));
      m_columnNameMap.insert(std::make_pair(index++, L));
      m_columnNameMap.insert(std::make_pair(index++, DSPACING));
      m_columnNameMap.insert(std::make_pair(index++, INT));
      m_columnNameMap.insert(std::make_pair(index++, SIGMINT));
      m_columnNameMap.insert(std::make_pair(index++, QLAB));
      m_columnNameMap.insert(std::make_pair(index++, QSAMPLE));

      m_sortableColumns.insert(std::make_pair(RUNNUMBER, true));
      m_sortableColumns.insert(std::make_pair(DETID, true));
      m_sortableColumns.insert(std::make_pair(H,true));
      m_sortableColumns.insert(std::make_pair(K,true));
      m_sortableColumns.insert(std::make_pair(L,true));
      m_sortableColumns.insert(std::make_pair(DSPACING, true));
      m_sortableColumns.insert(std::make_pair(INT, true));
      m_sortableColumns.insert(std::make_pair(SIGMINT, true));
      m_sortableColumns.insert(std::make_pair(QLAB, false));
      m_sortableColumns.insert(std::make_pair(QSAMPLE, false));

      if (!Mantid::Kernel::ConfigService::Instance().getValue("PeakColumn.hklPrec", m_hklPrec))
        m_hklPrec = 2;
    }

    /**
    Update the model.
    */
    void QPeaksTableModel::update()
    {
      emit layoutChanged(); //This should tell the view that the data has changed.
    }

    /**
    @return the row count.
    */
    int QPeaksTableModel::rowCount(const QModelIndex &) const
    {
      return static_cast<int>(m_peaksWS->rowCount());
    }

    /**
    @return the number of columns in the model.
    */
    int QPeaksTableModel::columnCount(const QModelIndex &) const
    {
      return static_cast<int>(m_columnNameMap.size());
    }

    /**
    Find the column name at a given column index.
    @param colIndex : Index to find column name for.
    */
    QString QPeaksTableModel::findColumnName(const int colIndex) const
    {
      ColumnIndexNameMap::const_iterator foundColName = m_columnNameMap.find(colIndex);
      if(foundColName == m_columnNameMap.end())
      {
        throw std::runtime_error("Unknown column requested");
      }
      return foundColName->second;
    }

    /**
    Overrident data method, allows consuming view to extract data for an index and role.
    @param index : For which to extract the data
    @param role : Role mode
    */
    QVariant QPeaksTableModel::data(const QModelIndex &index, int role) const
    {

      if (role == Qt::TextAlignmentRole)
        return Qt::AlignRight;

      if( role != Qt::DisplayRole ) 
        return QVariant();

      const int colNumber = index.column();
      const int rowNumber = index.row();

      const IPeak& peak = m_peaksWS->getPeak(rowNumber);
      this->updateDataCache(peak, rowNumber);
      return m_dataCache[colNumber];
    }

    /**
    Get the heading for a given section, orientation and role.
    @param section : Column index
    @param orientation : Heading orientation
    @param role : Role mode of table.
    @return HeaderData.
    */
    QVariant QPeaksTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
       if (role != Qt::DisplayRole)
        return QVariant();

      if (orientation == Qt::Horizontal) 
      {
        return findColumnName(section);
      }
      return QVariant();
    }

    /**
    Provide flags on an index by index basis
    @param index: To generate a flag for.
    */
    Qt::ItemFlags QPeaksTableModel::flags(const QModelIndex &index) const
    {
      if (!index.isValid()) return 0;
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    /// Destructor
    QPeaksTableModel::~QPeaksTableModel()
    {
    }

    /**
     * Overriden sort.
     * @param column
     * @param order
     */
    void QPeaksTableModel::sort ( int column, Qt::SortOrder order )
    {
      using namespace Mantid::API;
      const QString columnName = findColumnName(column);
      const bool isSortable = m_sortableColumns[columnName];
      if(isSortable)
      {
       // TODO raise event and propagate through to Proper presenter.
       peaksSorted(columnName.toStdString(), order== Qt::AscendingOrder);

       emit layoutChanged(); //This should tell the view that the data has changed.
      }
    }
  }
}
