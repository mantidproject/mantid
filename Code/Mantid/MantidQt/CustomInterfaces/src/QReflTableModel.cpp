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
      int index = 0;
      m_columnNameMap.insert(std::make_pair(index++, RUNS));
      m_columnNameMap.insert(std::make_pair(index++, ANGLE));
      m_columnNameMap.insert(std::make_pair(index++, TRANSMISSION));
      m_columnNameMap.insert(std::make_pair(index++, QMIN));
      m_columnNameMap.insert(std::make_pair(index++, QMAX));
      m_columnNameMap.insert(std::make_pair(index++, DQQ));
      m_columnNameMap.insert(std::make_pair(index++, SCALE));
      m_columnNameMap.insert(std::make_pair(index++, GROUP));
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    QReflTableModel::~QReflTableModel()
    {
    }

    void QReflTableModel::updateDataCache(const int row) const
    {
      // if the index is what is already cached just return
      if (row == m_dataCachePeakIndex)
        return;

      TableRow tableRow = m_tWS->getRow(row);

      // generate the cache
      m_dataCache.clear();
      m_dataCache.push_back(QString(tableRow.cell<std::string>(0).c_str()));
      m_dataCache.push_back(QString(tableRow.cell<std::string>(1).c_str()));
      m_dataCache.push_back(QString(tableRow.cell<std::string>(2).c_str()));
      m_dataCache.push_back(QString(tableRow.cell<std::string>(3).c_str()));
      m_dataCache.push_back(QString(tableRow.cell<std::string>(4).c_str()));
      m_dataCache.push_back(QString(tableRow.cell<std::string>(5).c_str()));
      m_dataCache.push_back(QString(tableRow.cell<std::string>(6).c_str()));
      m_dataCache.push_back(QString::number(tableRow.cell<int>(7)));

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
        return Qt::AlignRight;

      if( role != Qt::DisplayRole )
        return QVariant();

      const int colNumber = index.column();
      const int rowNumber = index.row();

      this->updateDataCache(rowNumber);
      return m_dataCache[colNumber];
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
      return QVariant();
    }

    /**
    Provide flags on an index by index basis
    @param index: To generate a flag for.
    */
    Qt::ItemFlags QReflTableModel::flags(const QModelIndex &index) const
    {
      if (!index.isValid()) return 0;
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

  } // namespace CustomInterfaces
} // namespace Mantid
