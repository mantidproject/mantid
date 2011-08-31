#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h" 
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Constructor
    WorkspaceMementoCollection::WorkspaceMementoCollection()
    {
      using namespace Mantid::API;
      //Call workspace factory to populate m_data.
      m_data = WorkspaceFactory::Instance().createTable("TableWorkspace");

      //Define the table schema here
      m_data->addColumn("str", "WorkspaceName");
      //TODO, lots more columns will be required here.
    }

    /**
    Register a workspace so that mementos may be generated from it.
    @param ws : ref to workspace to generate mementos for.
    */
    void WorkspaceMementoCollection::registerWorkspace(const Mantid::API::Workspace& ws)
    {
      using namespace Mantid::API;
      //Add row to table.
      m_data->insertRow(m_data->rowCount() + 1);
      TableRow row = m_data->getRow(m_data->rowCount()-1);
      row << ws.getName();
    }

    /**
    Gets the workspace memento at a designated runnumber.
    Workspace mementos are returned in locking smart pointers to ensure that there is no accidental duplicate access.
    @param runNumber : The run number.
    @return locking smart pointer wrapping workspace memento.
    */
    LockingMemento WorkspaceMementoCollection::at(int runNumber)
    {
      WorkspaceMemento* memento = new WorkspaceMemento(m_data, runNumber);
      
      //Table schema should be identical to that described in constructor.
      
      memento->addItem(new WorkspaceMementoItem<0, std::string>(m_data, runNumber));
      //TODO lots more column items to add.
      
      //Wrap product and return.
      return LockingMemento(memento);
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
    }

  }
}