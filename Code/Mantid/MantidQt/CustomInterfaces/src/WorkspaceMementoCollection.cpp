#include "MantidKernel/Property.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h" 
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidQtCustomInterfaces/Updateable.h"

#include <utility>
#include <string>

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /// Helper to handle changes in mode according to the workspace information available.
    class ActionManager
    {
    private:
      StatusOptions _status;
    public:
      ActionManager() : _status(ReadyForAnything) {}
      void ask(StatusOptions request)
      {
        if (request < _status) //Only permit downgrades.
        {
          _status = request;
        }
      }
      StatusOptions status()  const {return _status;}
    };

    /// Constructor
    WorkspaceMementoCollection::WorkspaceMementoCollection()
    {
      using namespace Mantid::API;
      //Call workspace factory to populate m_data.
      m_data = WorkspaceFactory::Instance().createTable("MementoTableWorkspace");

    }

    /**
    Register a workspace so that mementos may be generated from it.
    @param ws : ref to workspace to generate mementos for.
    @param model : ptr to the model which can be updated.
    */
    void WorkspaceMementoCollection::registerWorkspace(Mantid::API::MatrixWorkspace_const_sptr ws, Updateable* model)
    {
      ActionManager mode;
      int rowIndex = m_data->rowCount() + 1;
      m_data->insertRow(rowIndex);

      WorkspaceMemento* temp = new WorkspaceMemento(m_data, "Temp");
      WorkspaceMementoService<WorkspaceMemento*> service(temp);
      service.addAllItems(m_data, rowIndex-1);
      service.setWorkspaceName(ws->getName());
      service.setInstrumentName(ws->getInstrument()->getName());
      service.setRunNumber(ws->getRunNumber());
      if(ws->sample().getShape().hasValidShape())
      {
        service.setShapeXML(ws->sample().getShape().getShapeXML());
      }
      else
      {
        service.setShapeXML("");
        mode.ask(ReadyForPlotting);
      }
      if(ws->sample().hasOrientedLattice())
      {
        OrientedLattice lattice = ws->sample().getOrientedLattice();
        service.setLatticeParameters(lattice.a(), lattice.b(), lattice.c(), lattice.alpha(), lattice.beta(), lattice.gamma());
      }
      else
      {
        service.setLatticeParameters(0, 0, 0, 0, 0, 0);
        mode.ask(NotReady); 
      }
      std::string status;
      if(mode.status() == ReadyForPlotting)
      {
        status = "Ready For Plotting";
      }
      else if(mode.status() == ReadyForAnything)
      {
        status = "Ready For Anything";
      }
      else
      {
        status = "Not Ready";
      }
      service.setStatus(status);
      temp->commit(); 

      // Append all log data here.
      //typedef std::vector<Mantid::Kernel::Property*> VecLogType;
      //VecLogType logData = ws->run().getLogData();
      //service.setLogData(logData);

      //Make a mutable record by adding to a mutable member.

      model->update();
    }

    Mantid::API::ITableWorkspace_sptr WorkspaceMementoCollection::getWorkingData() const
    {
      return m_data;
    }

    void WorkspaceMementoCollection::unregisterWorkspace(const std::string& wsName, Updateable* model)
    {
      //Clear the table row.
      int row;
      m_data->find(wsName, row, 0);
      m_data->removeRow(row);

      //Clear any memento map entry.
      MementoMap::iterator it = m_mementoMap.find(wsName);
      if(m_mementoMap.end() != it)
      {
        delete it->second;
        m_mementoMap.erase(it);
      }
      model->update();
    }

    void WorkspaceMementoCollection::revertAll(Updateable* model)
    {
      MementoMap::iterator it = m_mementoMap.begin();
      while(it != m_mementoMap.end())
      {
        it->second->rollback();
      }
      model->update();
    }

    void WorkspaceMementoCollection::applyAll(Updateable* model)
    {
      MementoMap::iterator it = m_mementoMap.begin();
      while(it != m_mementoMap.end())
      {
        it->second->commit();
      }
      model->update();
    }

    size_t WorkspaceMementoCollection::size()
    {
      return m_data->rowCount();
    }

    /**
    Gets the workspace memento at a designated runnumber.
    Mementos are leased-out, but owned by the collection.
    @param rowindex : The row index.
    @return locking smart pointer wrapping workspace memento.
    */
    LoanedMemento WorkspaceMementoCollection::at(int rowIndex)
    {

      /// Index by workspace name.
      std::string wsName = m_data->cell<std::string>(rowIndex, 0);
      if(m_mementoMap.end() == m_mementoMap.find(wsName))
      {
        WorkspaceMemento* memento = new WorkspaceMemento(m_data, wsName);
        m_mementoMap.insert(std::make_pair(wsName, memento));

        ///Use the helper service to configure the memento.
        LoanedMemento managedMemento(memento);
        WorkspaceMementoService<LoanedMemento> service(managedMemento);
        service.addAllItems(m_data, rowIndex);
      }
      //Wrap product and return.
      return LoanedMemento(m_mementoMap[wsName]);
    }

    /**
    Serializes workspace changes to xml.
    @return serialized table workspace containing all diffs.
    */
    Mantid::API::ITableWorkspace* WorkspaceMementoCollection::serialize() const
    {
      //TODO, should hasChanges be checked first? Presumbably we need to ensure that all mementos are in an unmodified state before this is allowed.
      return m_data->clone();
    }

    /// Destructor
    WorkspaceMementoCollection::~WorkspaceMementoCollection()
    {
      MementoMap::iterator it = m_mementoMap.begin();
      while(it != m_mementoMap.end())
      {
        delete it->second;
        it++;
      }
    }

  }
}