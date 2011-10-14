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
      const int nLogValueColumns = ws->columnCount() - m_logValueStart;
      if(ws->columnCount() < m_logValueStart)
      {
        throw std::runtime_error("Too few columns in table schema.");
      }
      m_memento->addItem(new WorkspaceMementoItem<0, std::string>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<1, std::string>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<2, int>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<3, std::string>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<4, double>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<5, double>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<6, double>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<7, double>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<8, double>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<9, double>(ws, rowIndex));
      m_memento->addItem(new WorkspaceMementoItem<10, std::string>(ws, rowIndex));
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
    void WorkspaceMementoService<Memento>::addLogItems(Mantid::API::ITableWorkspace_sptr ws, std::vector<Mantid::Kernel::Property*> vecLogData, int rowIndex)
    {
      typedef std::vector<Mantid::Kernel::Property*> VecLogType;
      VecLogType::iterator it = vecLogData.begin();
      int count = m_logValueStart;
      while(it != vecLogData.end())
      {
        ws->addColumn("str", (*it)->name());
        m_memento->addItem(new WorkspaceMementoItem<11, std::string>(ws, rowIndex));
        it++;
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