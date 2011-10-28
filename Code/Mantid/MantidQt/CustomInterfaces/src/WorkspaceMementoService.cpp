#include "MantidKernel/Property.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /*
    Constructor
    @param memento : The memento to provide the service for.
    */
    template<typename Memento>
    WorkspaceMementoService<Memento>::WorkspaceMementoService(Memento memento) : m_memento(memento), m_logValueStart(11)
    {
    }

    /// Destructor
    template<typename Memento>
    WorkspaceMementoService<Memento>::~WorkspaceMementoService()
    {
      //Cleanup policy
      typetraits<Memento>::cleanUp(m_memento);
    }

    /*
    Add all items onto the Memento based on the provided table workspace.
    @param ws : TableWorkspace to reference. Col and Row -> WsMementoItem
    @param rowIndex : The row in the table to index from.
    */
    template<typename Memento>
    void WorkspaceMementoService<Memento>::addAllItems(Mantid::API::ITableWorkspace_sptr ws, int rowIndex)
    {
      if(ws->columnCount() < m_logValueStart)
      {
        throw std::runtime_error("Too few columns in table schema.");
      }
      m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(0)));
      m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(1)));
      m_memento->addItem(new WorkspaceMementoItem<int>(ws, Row(rowIndex), Column(2)));
      m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(3)));
      m_memento->addItem(new WorkspaceMementoItem<double>(ws, Row(rowIndex), Column(4)));
      m_memento->addItem(new WorkspaceMementoItem<double>(ws, Row(rowIndex), Column(5)));
      m_memento->addItem(new WorkspaceMementoItem<double>(ws, Row(rowIndex), Column(6)));
      m_memento->addItem(new WorkspaceMementoItem<double>(ws, Row(rowIndex), Column(7)));
      m_memento->addItem(new WorkspaceMementoItem<double>(ws, Row(rowIndex), Column(8)));
      m_memento->addItem(new WorkspaceMementoItem<double>(ws, Row(rowIndex), Column(9)));
      m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(10)));
    }


    /*
    Validate a table schema.
    @param ws : checks each workspace coltype against each WorkspaceMementoItem type.
    @return true if valid, otherwise false.
    */
    template<typename Memento>
    bool WorkspaceMementoService<Memento>::validMementoTableSchema(Mantid::API::ITableWorkspace_sptr ws)
    {
       if(size_t(ws->columnCount()) != m_memento->getSize()) //Do the sizes match?::W
       {
         return false;
       }
       for(int i = 0 ; i< ws->columnCount(); i++)
       {
         if(ws->getColumn(i)->get_type_info() != m_memento->getItem(i)->get_type_info()) //Do the types match?
         {
           return false;
         }
       }
       return true;
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setWorkspaceName(std::string name) 
    {
      m_memento->getItem(0)->setValue(name);
    }

    template<typename Memento>
    std::string WorkspaceMementoService<Memento>::getWorkspaceName()
    {
      std::string arg;
      m_memento->getItem(0)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setInstrumentName(std::string name)
    {
      m_memento->getItem(1)->setValue(name);
    }

    template<typename Memento>
    std::string WorkspaceMementoService<Memento>::getInstrumentName()
    {
      std::string arg;
      m_memento->getItem(1)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setShapeXML(std::string shapeXML)
    {
      m_memento->getItem(3)->setValue(shapeXML);
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setRunNumber(int runnumber)
    {
      m_memento->getItem(2)->setValue(runnumber);
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::declareLogItem(std::string name)
    {
        std::vector<std::string> names = m_memento->getData()->getColumnNames();
        std::vector<std::string>::iterator pos = std::find(names.begin(), names.end(), name);
        if(pos == names.end())
        {
          m_memento->getData()->addColumn("str", name); //TODO. THIS IS A NON-REVERTABLE CHANGE TO THE UNDERYLING TABLE WORKSPACE
        }
        Row row(m_memento->getRowIndex());
        Column col(m_memento->getData()->columnCount()-1);
        m_memento->addItem(new WorkspaceMementoItem<std::string>(m_memento->getData(), row, col));
    }

    
    template<typename Memento>
    void WorkspaceMementoService<Memento>::declareLogItems(Mantid::API::ITableWorkspace_sptr ws, std::vector<Mantid::Kernel::Property*> vecLogData, int rowIndex)
    {
      typedef std::vector<Mantid::Kernel::Property*> VecLogType;
      VecLogType::iterator it = vecLogData.begin();
      int count = m_logValueStart;
      while(it != vecLogData.end())
      {
        std::vector<std::string> names = ws->getColumnNames();
        std::vector<std::string>::iterator pos = std::find(names.begin(), names.end(), (*it)->name());
        if(pos == names.end())
        {
          ws->addColumn("str", (*it)->name());
        }
        m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(count)));
        count++;
        it++;
      }
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::declareLogItems(Mantid::API::ITableWorkspace_sptr ws, std::vector<std::string> vecLogData, int rowIndex)
    {
      typedef std::vector<std::string> VecLogType;
      VecLogType::iterator it = vecLogData.begin();
      int count = m_logValueStart;
      while(it != vecLogData.end())
      {
        ws->addColumn("str", (*it)); 
        m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(count)));
        it++;
        count++;
      }
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::addLogItems(Mantid::API::ITableWorkspace_sptr ws, int rowIndex)
    {
      typedef std::vector<std::string> VecLogType;
      int count = m_logValueStart;
      while(count < ws->columnCount())
      {
        m_memento->addItem(new WorkspaceMementoItem<std::string>(ws, Row(rowIndex), Column(count)));
        count++;
      }
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setLogData(std::vector<Mantid::Kernel::Property*> vecLogData)
    {
      typedef std::vector<Mantid::Kernel::Property*> VecLogType;
      VecLogType::iterator it = vecLogData.begin();
      int count = m_logValueStart;
      while(it != vecLogData.end())
      {
        std::string value = (*it)->value();
        m_memento->getItem(count)->setValue(value);
        it++;
        count++;
      }
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setLogData(std::vector<std::string> vecLogData)
    {
      typedef std::vector<std::string> VecLogType;
      VecLogType::iterator it = vecLogData.begin();
      int count = m_logValueStart;
      while(it != vecLogData.end())
      {
        std::string value = (*it);
        m_memento->getItem(count)->setValue(value);
        it++;
        count++;
      }
    }

    template<typename Memento>
    std::vector<AbstractMementoItem_sptr> WorkspaceMementoService<Memento>::getLogData()
    {
      typedef std::vector<AbstractMementoItem_sptr> VecLogType;
      VecLogType vecLogData;

      for(int i = m_logValueStart; i < static_cast<int>(m_memento->getSize()); i++)
      {
        vecLogData.push_back(m_memento->getItem(i));
      }
      return vecLogData;
    }

    template<typename Memento>
    int WorkspaceMementoService<Memento>::getRunNumber()
    {
      int arg;
      m_memento->getItem(2)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    std::string WorkspaceMementoService<Memento>::getShapeXML()
    {
      std::string arg;
      m_memento->getItem(3)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setLatticeParameters(double a1, double a2, double a3, double b1, double b2, double b3)
    {
      m_memento->getItem(4)->setValue(a1);
      m_memento->getItem(5)->setValue(a2);
      m_memento->getItem(6)->setValue(a3);
      m_memento->getItem(7)->setValue(b1);
      m_memento->getItem(8)->setValue(b2);
      m_memento->getItem(9)->setValue(b3);
    }

    template<typename Memento>
    void WorkspaceMementoService<Memento>::setStatus(std::string status)
    {
      m_memento->getItem(10)->setValue(status);
    }

    template<typename Memento>
    double WorkspaceMementoService<Memento>::getA1()
    {
      double arg;
      m_memento->getItem(4)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    double WorkspaceMementoService<Memento>::getA2()
    {
      double arg;
      m_memento->getItem(5)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    double WorkspaceMementoService<Memento>::getA3()
    {
      double arg;
      m_memento->getItem(6)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    double WorkspaceMementoService<Memento>::getB1()
    {
      double arg;
      m_memento->getItem(7)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    double WorkspaceMementoService<Memento>::getB2()
    {
      double arg;
      m_memento->getItem(8)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    double WorkspaceMementoService<Memento>::getB3()
    {
      double arg;
      m_memento->getItem(9)->getValue(arg);
      return arg;
    }

    template<typename Memento>
    std::string WorkspaceMementoService<Memento>::getStatus()
    {
      std::string arg;
      m_memento->getItem(10)->getValue(arg);
      return arg;
    }


    /*Templated Types*/
    template class WorkspaceMementoService<LoanedMemento>;
    template class WorkspaceMementoService<WorkspaceMemento*>;
  }
}