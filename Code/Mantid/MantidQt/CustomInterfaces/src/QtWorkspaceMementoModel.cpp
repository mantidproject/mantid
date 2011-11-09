#include "MantidQtCustomInterfaces/QtWorkspaceMementoModel.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt
{
namespace CustomInterfaces
{

    QtWorkspaceMementoModel::QtWorkspaceMementoModel(Mantid::API::ITableWorkspace_sptr displayData) : 
      QAbstractTableModel(NULL), m_displayData(displayData)
    {
    }

    void QtWorkspaceMementoModel::update()
    {
      emit layoutChanged(); //This should tell the view that the data has changed.
    }

    int QtWorkspaceMementoModel::rowCount(const QModelIndex&) const
    { 
      return m_displayData->rowCount();
    }

    int QtWorkspaceMementoModel::columnCount(const QModelIndex&) const
    {
      return 4; //Only display a subset of the actual available columns in the table workspace.
    }

    QVariant QtWorkspaceMementoModel::data(const QModelIndex &index, int role) const
    {
      if( role != Qt::DisplayRole ) return QVariant();
      Mantid::API::Column_sptr col;
      const int colNumber = index.column();

      /*Here we are effectively providing a view over the underlying data, selecting a sub-set of columns to show.
      mapping is from colNumber in view to column in table workspace.*/
      switch(colNumber)
      {
      case 3:
        col = m_displayData->getColumn(10);
        break;
      default:
        col = m_displayData->getColumn(colNumber);
        break;
      }
      std::stringstream strstream;
      col->print(strstream, index.row());
      return QString(strstream.str().c_str());
    }

    QVariant QtWorkspaceMementoModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
      if (role != Qt::DisplayRole)
        return QVariant();

      if (orientation == Qt::Horizontal) 
      {
        //List the headings for each section.
        switch (section) 
        {
        case 0:
          return "Workspace Name";
        case 1:
          return "Instrument Name";
        case 2:
          return "Run Number";
        case 3:
          return "Status";

        default:
          return QVariant();
        }
      }
      return QVariant();
    }

    Qt::ItemFlags QtWorkspaceMementoModel::flags(const QModelIndex &index) const
    {
      if (!index.isValid()) return 0;

      return Qt::ItemIsEnabled;
    }

    QtWorkspaceMementoModel::~QtWorkspaceMementoModel()
    {
    }

  }
}