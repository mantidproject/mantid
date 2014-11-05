#include "MantidQtCustomInterfaces/ReflSearchModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::API;

    //----------------------------------------------------------------------------------------------
    /** Constructor
    @param tableWorkspace : The table workspace to wrap
    */
    ReflSearchModel::ReflSearchModel(ITableWorkspace_sptr tableWorkspace) : m_tWS(tableWorkspace)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflSearchModel::~ReflSearchModel()
    {
    }

    /**
    @return the row count.
    */
    int ReflSearchModel::rowCount(const QModelIndex &) const
    {
      return static_cast<int>(m_tWS->rowCount());
    }

    /**
    @return the number of columns in the model.
    */
    int ReflSearchModel::columnCount(const QModelIndex &) const
    {
      return 2;
    }

    /**
    Overrident data method, allows consuming view to extract data for an index and role.
    @param index : For which to extract the data
    @param role : Role mode
    */
    QVariant ReflSearchModel::data(const QModelIndex &index, int role) const
    {
      if(role != Qt::DisplayRole)
        return QVariant();

      const int colNumber = index.column();
      const int rowNumber = index.row();

      TableRow tableRow = m_tWS->getRow(rowNumber);
      //run column, or description column
      return QString::fromStdString(tableRow.cell<std::string>(colNumber == 0 ? 0 : 6));
    }

    /**
    Get the heading for a given section, orientation and role.
    @param section : Column index
    @param orientation : Heading orientation
    @param role : Role mode of table.
    @return HeaderData.
    */
    QVariant ReflSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
      if(role != Qt::DisplayRole)
        return QVariant();

      if(orientation == Qt::Horizontal)
      {
        switch(section)
        {
          case 0: return "Run";
          case 1: return "Description";
          default: return "";
        }
      }
      return QVariant();
    }

    /**
    Provide flags on an index by index basis
    @param index: To generate a flag for.
    */
    Qt::ItemFlags ReflSearchModel::flags(const QModelIndex &index) const
    {
      if(!index.isValid())
        return 0;
      else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
  } // namespace CustomInterfaces
} // namespace Mantid
