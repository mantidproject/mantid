#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h" 
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"
#include "MantidQtCustomInterfaces/ExternalDrivenModel.h"

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
    void WorkspaceMementoCollection::registerWorkspace(Mantid::API::MatrixWorkspace_const_sptr ws, ExternalDrivenModel* model)
    {
      enum statusoptions{notready, readyforvisualisation, notreadyforanything};
      using namespace Mantid::API;

      ActionManager mode;
      m_data->insertRow(m_data->rowCount() + 1);
      TableRow row = m_data->getRow(m_data->rowCount()-1);
      row << ws->getName() 
        << ws->getInstrument()->getName() 
        << ws->getRunNumber();

      if(ws->sample().getShape().hasValidShape())
      {
        row << ws->sample().getShape().getShapeXML();
      }
      else
      {
        row << "";
        mode.ask(ReadyForPlotting);
      }
      if(ws->sample().hasOrientedLattice())
      {
        row << ws->sample().getOrientedLattice().a()
          << ws->sample().getOrientedLattice().b()
          << ws->sample().getOrientedLattice().c()
          << ws->sample().getOrientedLattice().alpha()
          << ws->sample().getOrientedLattice().beta()
          << ws->sample().getOrientedLattice().gamma();
      }
      else
      {
        row << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
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
      row << status;

      // Append all log data here.
      //typedef std::vector<Kernel::Property*> VecLogType;
      //VecLogType logData = matrixWs->getLogData();
      //VecLogType::iterator it = logData.begin();
      //while(it != logData.end())
      //{
      //  row << it->value();
      //  it++;
      //}

      //Make a mutable record by adding to a mutable member.

      model->update();
    }

    Mantid::API::ITableWorkspace_sptr WorkspaceMementoCollection::getWorkingData() const
    {
      return m_data;
    }

    void WorkspaceMementoCollection::unregisterWorkspace(const std::string& wsName, ExternalDrivenModel* model)
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

    void WorkspaceMementoCollection::revertAll(ExternalDrivenModel* model)
    {
      MementoMap::iterator it = m_mementoMap.begin();
      while(it != m_mementoMap.end())
      {
        it->second->rollback();
      }
      model->update();
    }

    void WorkspaceMementoCollection::applyAll(ExternalDrivenModel* model)
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


        //Table schema should be identical to that described in constructor.

        memento->addItem(new WorkspaceMementoItem<0, std::string>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<1, std::string>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<2, int>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<3, std::string>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<4, double>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<5, double>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<6, double>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<7, double>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<8, double>(m_data, rowIndex));
        memento->addItem(new WorkspaceMementoItem<9, double>(m_data, rowIndex));
        //TODO lots more column items to add.
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