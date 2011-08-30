#ifndef MANTID_DATAOBJECTS_MEMENTO_COLLECTION_H_
#define MANTID_DATAOBJECTS_MEMENTO_COLLECTION_H_

#include "MantidDataObjects/WorkspaceMemento.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace API
  {
    class Workspace;
  }
  namespace DataObjects
  {
    /// Locking proxy smart pointer. Ensures that Workspace Mementos are always locked and unlocked.
    class DLLExport LockingMemento {
    public:
      LockingMemento (WorkspaceMemento *memento) : m_memento (memento) 
      {
        m_memento->lock();
      }
      WorkspaceMemento * operator -> () 
      {
        return m_memento;
      }
      ~LockingMemento () 
      {
        m_memento->unlock();
        delete m_memento;
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
    The Memento collection does NOT own memento objects, it is a factory for them.

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
    class DLLExport WorkspaceMementoCollection
    {
    public:
      WorkspaceMementoCollection();
      ~WorkspaceMementoCollection();
      void registerWorkspace(const Mantid::API::Workspace& ws);
      LockingMemento at(size_t runNumber);
      TableWorkspace* serialize() const;
    private:
      TableWorkspace m_data;
    };
  }
}

#endif