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
    const QString QReflTableModel::TRANSMISSION = "Transmission Run(s)";
    const QString QReflTableModel::QMIN = "Q min";
    const QString QReflTableModel::QMAX = "Q max";
    const QString QReflTableModel::DQQ = "dQ/Q";
    const QString QReflTableModel::SCALE = "Scale";
    const QString QReflTableModel::GROUP = "Group";
    const QString QReflTableModel::OPTIONS = "Options";

    const int QReflTableModel::COL_RUNS(0);
    const int QReflTableModel::COL_ANGLE(1);
    const int QReflTableModel::COL_TRANSMISSION(2);
    const int QReflTableModel::COL_QMIN(3);
    const int QReflTableModel::COL_QMAX(4);
    const int QReflTableModel::COL_DQQ(5);
    const int QReflTableModel::COL_SCALE(6);
    const int QReflTableModel::COL_GROUP(7);
    const int QReflTableModel::COL_OPTIONS(8);

    //----------------------------------------------------------------------------------------------
    /** Constructor
    @param tableWorkspace : The table workspace to wrap
    */
    QReflTableModel::QReflTableModel(ITableWorkspace_sptr tableWorkspace) : m_dataCachePeakIndex(-1), m_tWS(tableWorkspace)
    {
      m_columnNameMap.insert(std::make_pair(COL_RUNS, RUNS));
      m_columnNameMap.insert(std::make_pair(COL_ANGLE, ANGLE));
      m_columnNameMap.insert(std::make_pair(COL_TRANSMISSION, TRANSMISSION));
      m_columnNameMap.insert(std::make_pair(COL_QMIN, QMIN));
      m_columnNameMap.insert(std::make_pair(COL_QMAX, QMAX));
      m_columnNameMap.insert(std::make_pair(COL_DQQ, DQQ));
      m_columnNameMap.insert(std::make_pair(COL_SCALE, SCALE));
      m_columnNameMap.insert(std::make_pair(COL_GROUP, GROUP));
      m_columnNameMap.insert(std::make_pair(COL_OPTIONS, OPTIONS));
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
      if(row == m_dataCachePeakIndex || row == -1)
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
      m_dataCache.push_back(QString::number(tableRow.cell<double>(COL_SCALE)));
      m_dataCache.push_back(QString::number(tableRow.cell<int>(COL_GROUP)));
      m_dataCache.push_back(QString::fromStdString(tableRow.cell<std::string>(COL_OPTIONS)));

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
      if(role == Qt::TextAlignmentRole)
      {
        if(index.column() == COL_OPTIONS)
          return Qt::AlignLeft;
        else
          return Qt::AlignRight;
      }
      else if(role != Qt::DisplayRole && role != Qt::EditRole)
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
    @param value : New value
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

        switch(colNumber)
        {
        case COL_GROUP:
          m_tWS->Int(rowNumber, COL_GROUP) = str.toInt(); break;
        case COL_SCALE:
          m_tWS->Double(rowNumber, COL_SCALE) = str.toDouble(); break;
        default:
          m_tWS->String(rowNumber, colNumber) = str.toStdString(); break;
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
      if(role == Qt::WhatsThisRole && orientation == Qt::Horizontal)
      {
        switch(section)
        {
          case COL_RUNS:
            return QString(
                "<b>Sample runs to be processed.</b><br />"
                "<i>required</i><br />"
                "Runs may be given as run numbers or workspace names. "
                "Multiple runs may be added together by separating them with a '+'. "
                "<br /><br /><b>Example:</b> <samp>1234+1235+1236</samp>"
                );
          case COL_ANGLE:
            return QString(
                "<b>Angle used during the run.</b><br />"
                "<i>optional</i><br />"
                "Unit: degrees<br />"
                "If left blank, this is set to the last value for 'THETA' in the run's sample log. "
                "If multiple runs were given in the Run(s) column, the first listed run's sample log will be used. "
                "<br /><br /><b>Example:</b> <samp>0.7</samp>"
                );
          case COL_TRANSMISSION:
            return QString(
                "<b>Transmission run(s) to use to normalise the sample runs.</b><br />"
                "<i>optional</i><br />"
                "To specify two transmission runs, separate them with a comma. "
                "If left blank, the sample runs will be normalised by monitor only."
                "<br /><br /><b>Example:</b> <samp>1234,12345</samp>"
                );
          case COL_QMIN:
            return QString(
                "<b>Minimum value of Q to be used</b><br />"
                "<i>optional</i><br />"
                "Unit: &#197;<sup>-1</sup><br />"
                "Data with a value of Q lower than this will be discarded. "
                "If left blank, this is set to the lowest Q value found. "
                "This is useful for discarding noisy data. "
                "<br /><br /><b>Example:</b> <samp>0.1</samp>"
                );
          case COL_QMAX:
            return QString(
                "<b>Maximum value of Q to be used</b><br />"
                "<i>optional</i><br />"
                "Unit: &#197;<sup>-1</sup><br />"
                "Data with a value of Q higher than this will be discarded. "
                "If left blank, this is set to the highest Q value found. "
                "This is useful for discarding noisy data. "
                "<br /><br /><b>Example:</b> <samp>0.9</samp>"
                );
          case COL_DQQ:
            return QString(
                "<b>Resolution used when rebinning</b><br />"
                "<i>optional</i><br />"
                "If left blank, this is calculated for you using the CalculateResolution algorithm. "
                "<br /><br /><b>Example:</b> <samp>0.9</samp>"
                );
          case COL_SCALE:
            return QString(
                "<b>Scaling factor</b><br />"
                "<i>required</i><br />"
                "The created IvsQ workspaces will be Scaled by <samp>1/i</samp> where <samp>i</samp> is the value of this column."
                "<br /><br /><b>Example:</b> <samp>1</samp>"
                );
          case COL_GROUP:
            return QString(
                "<b>Grouping for stitching</b><br />"
                "<i>required</i><br />"
                "The value of this column determines which other rows this row's output will be stitched with. "
                "All rows with the same group number are stitched together. "
                );
          case COL_OPTIONS:
            return QString(
                "<b>Override <samp>ReflectometryReductionOneAuto</samp> properties</b><br />"
                "<i>optional</i><br />"
                "This column allows you to override the properties used when executing <samp>ReflectometryReductionOneAuto</samp>. "
                "Options are given as key=value pairs, separated by commas. "
                "Values containing commas must be quoted."
                "<br /><br /><b>Example:</b> <samp>StrictSpectrumChecking=0, RegionOfDirectBeam=\"0,2\", Params=\"1,2,3\"</samp>"
                );
          default:
            return QVariant();
        }
      }
      else if(role == Qt::DisplayRole)
      {
        if(orientation == Qt::Horizontal)
          return findColumnName(section);
        if(orientation == Qt::Vertical)
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

    /**
    Insert the given number of rows at the specified position
    @param row : The row to insert before
    @param count : The number of rows to insert
    @param parent : The parent index
    */
    bool QReflTableModel::insertRows(int row, int count, const QModelIndex& parent)
    {
      if(count < 1)
        return true;

      if(row < 0)
        return false;

      beginInsertRows(parent, row, row + count - 1);
      for(int i = 0; i < count; ++i)
        m_tWS->insertRow(row + i);
      endInsertRows();

      invalidateDataCache(-1);
      return true;
    }

    /**
    Remove the given number of rows from the specified position
    @param row : The row index to remove from
    @param count : The number of rows to remove
    @param parent : The parent index
    */
    bool QReflTableModel::removeRows(int row, int count, const QModelIndex& parent)
    {
      if(count < 1)
        return true;

      if(row < 0)
        return false;

      beginRemoveRows(parent, row, row + count - 1);
      for(int i = 0; i < count; ++i)
        m_tWS->removeRow(row);
      endRemoveRows();

      invalidateDataCache(-1);
      return true;
    }

  } // namespace CustomInterfaces
} // namespace Mantid
