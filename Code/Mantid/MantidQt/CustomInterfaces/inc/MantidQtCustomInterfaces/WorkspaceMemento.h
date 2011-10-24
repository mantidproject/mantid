#ifndef MANTID_CUSTOMINTERFACES_MEMENTO_H_
#define MANTID_CUSTOMINTERFACES_MEMENTO_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"
#include <vector>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>


namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class WorkspaceMemento

    Stores local changes to Workspace metadata. Can commit or rollback these changes to an underlying table workspace,
    which encapsulates all changes to a set of related workspaces (see WorkspaceMementoCollection).

    @author Owen Arnold
    @date 30/08/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class WorkspaceMementoLock;
    class DLLExport WorkspaceMemento
    {
    public:

      typedef std::vector<AbstractMementoItem_sptr> VecMementoItems;
      WorkspaceMemento(Mantid::API::ITableWorkspace_sptr ws, std::string m_wsName, int rowIndex);
      WorkspaceMemento(Mantid::API::ITableWorkspace_sptr ws, std::string m_wsName, WorkspaceMementoLock* lock);
      ~WorkspaceMemento(); 
      void addItem(AbstractMementoItem* item);
      AbstractMementoItem_sptr getItem(const size_t col) const;
      void commit();
      void rollback();
      bool hasChanged() const;
      void validate() const;
      bool equals(const WorkspaceMemento& workspace) const;
      bool operator==(const WorkspaceMemento& workspace) const;
      bool operator!=(const WorkspaceMemento& workspace) const;
      bool locked() const;
      bool unlock();
      void lock();
      Mantid::API::ITableWorkspace_sptr getData() const;
      int getRowIndex() const;

      /// Getter for the size of this memento in terms of the number of items.
      size_t getSize()
      {
        return m_items.size();
      }

    private:
      /// Reference to underlying data.
      Mantid::API::ITableWorkspace_sptr m_data;
      /// Memento items to store local/un-saved changes.
      VecMementoItems m_items;
      /// Flag indicating memento is valid.
      bool m_validMemento;
      /// Name of the workspace wrapped.
      std::string m_wsName;
      /// Disabled copy constructor.
      WorkspaceMemento(const WorkspaceMemento&);
      /// Disabled assignement operator.
      WorkspaceMemento& operator=(const WorkspaceMemento&);
      /// Locking object.
      boost::scoped_ptr<WorkspaceMementoLock> m_lock;
      /// Index of corresponding row in the workspace.
      int m_rowIndex;

    };
  }
}

#endif