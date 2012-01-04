#include "MantidQtCustomInterfaces/QtWorkspaceMementoModel.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt
{
namespace CustomInterfaces
{

    QtWorkspaceMementoModel::QtWorkspaceMementoModel(const WorkspaceMementoCollection& displayData) : 
      QAbstractTableModel(NULL), m_displayData(displayData)
    {
    }

    void QtWorkspaceMementoModel::update()
    {
      emit layoutChanged(); //This should tell the view that the data has changed.
    }

    int QtWorkspaceMementoModel::rowCount(const QModelIndex&) const
    { 
      return static_cast<int>(m_displayData.size());
    }

    int QtWorkspaceMementoModel::columnCount(const QModelIndex&) const
    {
      return 3; 
    }

    QVariant QtWorkspaceMementoModel::data(const QModelIndex &index, int role) const
    {
      if( role != Qt::DisplayRole ) return QVariant();
      Mantid::API::Column_sptr col;
      const int colNumber = index.column();
      const int rowNumber = index.row();
      
      std::stringstream strstream;
      switch(colNumber)
      {
      case 0:
        strstream << m_displayData[rowNumber]->getId();
        break;
      case 1:
        strstream << m_displayData[rowNumber]->locationType();
        break;
      case 2:
        strstream << m_displayData[rowNumber]->statusReport();
        break;
      default:
        throw std::runtime_error("Unknown column requested");
      }
      
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
          return "Id/File name";
        case 1:
          return "Location";
        case 2:
          return "Status Report";
        default:
          throw std::runtime_error("Unknown column requested");
        }
      }
      return QVariant();
    }

    Qt::ItemFlags QtWorkspaceMementoModel::flags(const QModelIndex &index) const
    {
      if (!index.isValid()) return 0;

      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    QtWorkspaceMementoModel::~QtWorkspaceMementoModel()
    {
    }

  }
}