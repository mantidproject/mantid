#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::API;

    const QString QReflTableModel::RUNS = "Run(s)";
    const QString QReflTableModel::ANGLE = "Angle";
    const QString QReflTableModel::TRANSMISSION = "Transmission";
    const QString QReflTableModel::QMIN = "q_min";
    const QString QReflTableModel::QMAX = "q_max";
    const QString QReflTableModel::DQQ = "dq/q";
    const QString QReflTableModel::SCALE = "Scale";
    const QString QReflTableModel::GROUP = "Group";

    const int QReflTableModel::COL_RUNS(0);
    const int QReflTableModel::COL_ANGLE(1);
    const int QReflTableModel::COL_TRANSMISSION(2);
    const int QReflTableModel::COL_QMIN(3);
    const int QReflTableModel::COL_QMAX(4);
    const int QReflTableModel::COL_DQQ(5);
    const int QReflTableModel::COL_SCALE(6);
    const int QReflTableModel::COL_GROUP(7);

    //----------------------------------------------------------------------------------------------
    /** Constructor
    @param peaksWS : Workspace model.
    */
    QReflTableModel::QReflTableModel(ITableWorkspace_sptr tWS) : m_dataCachePeakIndex(-1), m_tWS(tWS)
    {
      m_columnNameMap.insert(std::make_pair(COL_RUNS, RUNS));
      m_columnNameMap.insert(std::make_pair(COL_ANGLE, ANGLE));
      m_columnNameMap.insert(std::make_pair(COL_TRANSMISSION, TRANSMISSION));
      m_columnNameMap.insert(std::make_pair(COL_QMIN, QMIN));
      m_columnNameMap.insert(std::make_pair(COL_QMAX, QMAX));
      m_columnNameMap.insert(std::make_pair(COL_DQQ, DQQ));
      m_columnNameMap.insert(std::make_pair(COL_SCALE, SCALE));
      m_columnNameMap.insert(std::make_pair(COL_GROUP, GROUP));
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    QReflTableModel::~QReflTableModel()
    {
    }

    /**
    Invalidate the cache for a row
    @param row : the row the cache needs to be invalidated for
    */
    void QReflTableModel::invalidateDataCache(const int row) const
    {
      //If the row is in the cache, invalidate the cache.
      if(row == m_dataCachePeakIndex)
        m_dataCachePeakIndex = -1;
    }

    /**
    Load data into the cache if required
    @param row : to check and load if required
    */
    void QReflTableModel::updateDataCache(const int row) const
    {
      // if the index is what is already cached just return
      if (row == m_dataCachePeakIndex)
        return;

      TableRow tableRow = m_tWS->getRow(row);

      // generate the cache
      m_dataCache.clear();
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_RUNS)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_ANGLE)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_TRANSMISSION)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_QMIN)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_QMAX)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_DQQ)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_SCALE)));
      m_dataCache.push_back(QString::number(tableRow.cell<int>(COL_GROUP)));

      m_dataCachePeakIndex = row;
    }

    /**
    Update the model.
    */
    void QReflTableModel::update()
    {
      emit layoutChanged(); //This should tell the view that the data has changed.
    }

    /**
    @return the row count.
    */
    int QReflTableModel::rowCount(const QModelIndex &) const
    {
      return static_cast<int>(m_tWS->rowCount());
    }

    /**
    @return the number of columns in the model.
    */
    int QReflTableModel::columnCount(const QModelIndex &) const
    {
      return static_cast<int>(m_columnNameMap.size());
    }

    /**
    Find the column name at a given column index.
    @param colIndex : Index to find column name for.
    */
    QString QReflTableModel::findColumnName(const int colIndex) const
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
    QVariant QReflTableModel::data(const QModelIndex &index, int role) const
    {
      if (role == Qt::TextAlignmentRole)
      {
        return Qt::AlignRight;
      }
      else if( role != Qt::DisplayRole && role != Qt::EditRole)
      {
        return QVariant();
      }
      const int colNumber = index.column();
      const int rowNumber = index.row();

      this->updateDataCache(rowNumber);
      return m_dataCache[colNumber];
    }

    /**
    Overrident setData method, allows view to set data for an index and role.
    @param index : For which to extract the data
    @param role : Role mode
    @returns booean true if sucessful, false if unsucessful.
    */
    bool QReflTableModel::setData ( const QModelIndex & index, const QVariant & value, int role)
    {
      //Users may mistakenly enter whitespace. Let's strip it for them.
      QString str = value.toString().trimmed();

      if (index.isValid() && role == Qt::EditRole)
      {
        const int colNumber = index.column();
        const int rowNumber = index.row();

        if (colNumber == COL_GROUP)
        {
          m_tWS->Int(rowNumber, COL_GROUP) = str.toInt();
        }
        else
        {
          m_tWS->String(rowNumber, colNumber) = str.toStdString();
        }

        invalidateDataCache(rowNumber);
        emit dataChanged(index, index);

        return true;
      }
      return false;
    }

    /**
    Get the heading for a given section, orientation and role.
    @param section : Column index
    @param orientation : Heading orientation
    @param role : Role mode of table.
    @return HeaderData.
    */
    QVariant QReflTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
      if (role != Qt::DisplayRole)
        return QVariant();

      if (orientation == Qt::Horizontal)
      {
        return findColumnName(section);
      }
      else if (orientation == Qt::Vertical)
      {
        return QString::number(section + 1);
      }
      return QVariant();
    }

    /**
    Provide flags on an index by index basis
    @param index: To generate a flag for.
    */
    Qt::ItemFlags QReflTableModel::flags(const QModelIndex &index) const
    {
      if (!index.isValid()) return 0;
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

  } // namespace CustomInterfaces
} // namespace Mantid
