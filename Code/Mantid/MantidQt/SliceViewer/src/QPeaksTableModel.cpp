#include <MantidQtSliceViewer/QPeaksTableModel.h>
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace boost;

namespace MantidQt
{
  namespace SliceViewer
  {
    const QString QPeaksTableModel::RUNNUMBER = "RunNumber";
    const QString QPeaksTableModel::DETID = "DetID";
    const QString QPeaksTableModel::HKL = "HKL";
    const QString QPeaksTableModel::DSPACING = "DSpacing";
    const QString QPeaksTableModel::INT = "Int";
    const QString QPeaksTableModel::SIGINT = "SigInt";
    const QString QPeaksTableModel::QLAB = "QLab";
    const QString QPeaksTableModel::QSAMPLE = "QSample";

    /**
    Create map. Create a map of column names to values for a peak.
    */
    QPeaksTableModel::ColumnNameRowValueMap QPeaksTableModel::createMap(const Mantid::API::IPeak& peak) const
    {
      ColumnNameRowValueMap map;
      map.insert(std::make_pair(RUNNUMBER, QString::number(peak.getRunNumber())));
      map.insert(std::make_pair(DETID, QString::number(peak.getDetectorID())));
      map.insert(std::make_pair(HKL, peak.getHKL().toString().c_str()));
      map.insert(std::make_pair(DSPACING, QString::number(peak.getDSpacing())));
      map.insert(std::make_pair(INT, QString::number(peak.getIntensity())));
      map.insert(std::make_pair(SIGINT, QString::number(peak.getIntensity())));
      map.insert(std::make_pair(QLAB, peak.getQLabFrame().toString().c_str()));
      map.insert(std::make_pair(QSAMPLE, peak.getQSampleFrame().toString().c_str()));
      return map;
    }

    /**
    Constructor
    @param ws : Workspace model.
    */
    QPeaksTableModel::QPeaksTableModel(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) : m_peaksWS(peaksWS)
    {
      int index = 0;
      m_columnNameMap.insert(std::make_pair(index++, RUNNUMBER));
      m_columnNameMap.insert(std::make_pair(index++, DETID));
      m_columnNameMap.insert(std::make_pair(index++, HKL));
      m_columnNameMap.insert(std::make_pair(index++, DSPACING));
      m_columnNameMap.insert(std::make_pair(index++, INT));
      m_columnNameMap.insert(std::make_pair(index++, SIGINT));
      m_columnNameMap.insert(std::make_pair(index++, QLAB));
      m_columnNameMap.insert(std::make_pair(index++, QSAMPLE));
    }

    /**
    Update the model.
    */
    void QPeaksTableModel::update()
    {
      // Do nothing. Read-only.
    }

    /**
    @return the row count.
    */
    int QPeaksTableModel::rowCount(const QModelIndex &parent) const
    {
      return static_cast<int>(m_peaksWS->rowCount());
    }

    /**
    @return the number of columns in the model.
    */
    int QPeaksTableModel::columnCount(const QModelIndex &parent) const
    {
      return static_cast<int>(m_columnNameMap.size());
    }

    /**
    Find the column name at a given column index.
    @colIndex : Index to find column name for.
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
      if( role != Qt::DisplayRole ) 
        return QVariant();

      const int colNumber = index.column();
      const int rowNumber = index.row();
      const IPeak& peak = m_peaksWS->getPeak(rowNumber);

      const QString colName = findColumnName(colNumber);

      ColumnNameRowValueMap valueMap = createMap(peak);
      ColumnNameRowValueMap::const_iterator foundRowValue = valueMap.find(colName);
      if(foundRowValue == valueMap.end())
      {
        throw std::runtime_error("Unknown row requested");
      }

      return foundRowValue->second;
    }

    /**
    Get the heading for a given section, orientation and role.
    @section : Column index
    @orientation : Heading orientation
    @role : Role mode of table.
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
    @index: To generate a flag for.
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
  }
}