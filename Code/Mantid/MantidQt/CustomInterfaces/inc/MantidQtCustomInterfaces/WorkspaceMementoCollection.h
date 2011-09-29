#ifndef MANTID_DATAOBJECTS_MEMENTO_COLLECTION_H_
#define MANTID_DATAOBJECTS_MEMENTO_COLLECTION_H_

#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <boost/scoped_ptr.hpp>
#include <map>

namespace MantidQt
{
  namespace API
  {
    class Workspace;
  }
  namespace CustomInterfaces
  {
    /// Smart pointer wraps mementos so that they can be borrowed, but not deleted 
    class DLLExport LoanedMemento {
    public:
      LoanedMemento (WorkspaceMemento *memento) : m_memento (memento) 
      {
        if(m_memento)
        {
          m_memento->lock();
        }
      }
      LoanedMemento(const LoanedMemento& other)
      {
        m_memento = other.m_memento;
        if(m_memento)
        {
          other.m_memento->unlock();
          m_memento->lock();
        }
      }
      LoanedMemento& operator=(LoanedMemento& other)
      {
        if(this != &other)
        {
          m_memento = other.m_memento;
          if(m_memento)
          {
            other.m_memento->unlock();
            m_memento->lock();
          }
        }
        return *this;
      }
      WorkspaceMemento * operator -> () 
      {
        return m_memento;
      }
      ~LoanedMemento () 
      {
        if(m_memento)
        {
          this->m_memento->unlock();
        }
        //Explicitly do not delete memento.
      }
    private:
      WorkspaceMemento* m_memento;
    };


    /** @class WorkspaceMementoCollection

    Stores all changes to registerd workspace in-terms of a diff-table (TableWorkspace). The final diff table can be exported and used
    to generate a series of workspaces encapsulating all the changes described within the table.

    Main workflow

    -Register workspaces via method call ::registerWorkspace
    -Fetch workspace mementos via method call ::at
    -Serialize changes via method call ::serialize

    Mementos are returned in a Locked format, meaning that no other memento may change that table row (corresponding to an individual run/workspace).
    Once the fields are changed on the memento, changes may be committed or rolled-back. These operations occur directly on the diff table contained within the MementoCollection.

    @author Owen Arnold
    @date 26/08/2011

    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    class ExternalDrivenModel;
    class DLLExport WorkspaceMementoCollection
    {
    public:
      WorkspaceMementoCollection();
      ~WorkspaceMementoCollection();
      void registerWorkspace(Mantid::API::MatrixWorkspace_const_sptr ws, ExternalDrivenModel* model);
      void unregisterWorkspace(const std::string& ws, ExternalDrivenModel* model);
      LoanedMemento at(int rowIndex);
      size_t size();
      Mantid::API::ITableWorkspace* serialize() const;
      void revertAll(ExternalDrivenModel* model);
      void applyAll(ExternalDrivenModel* model);
      boost::shared_ptr<Mantid::API::ITableWorkspace> getWorkingData() const;
    private:
      /// Type definition for a map of all mementos keyed via a name
      typedef std::map<std::string, WorkspaceMemento*> MementoMap;
      /// Map of all mementos keyed via a name
      MementoMap m_mementoMap;
      /// Table workspace data
      boost::shared_ptr<Mantid::API::ITableWorkspace> m_data;
    };
    /// Status descriptions.
    enum StatusOptions{NotReady, ReadyForPlotting, ReadyForAnything};
  }
}

#endif