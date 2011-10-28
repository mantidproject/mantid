#ifndef MANTIDCUSTOMINTERFACES_WORKSPACE_MEMENTO_SERVICE_H
#define MANTIDCUSTOMINTERFACES_WORKSPACE_MEMENTO_SERVICE_H 

#include "MantidKernel/System.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"

namespace Mantid
{
  namespace Kernel
  {
    class Property;
  }
  namespace API
  {
    class ITableWorkspace;
  }
}

    /** @class WorkspaceMementoService
    
    Very useful for providing a single point of configuration. The schemas of ITableWorkspaces, which are wrapped by WorkspaceMementos
    are volatile. We therefore do not want to replicate mappings throughout the code. We anticipate lot of changes in the table schema as the table 
    acts as the base storage type for all changes to all workspaces directed via the WorkspaceMementoCollection.

    This type is a service as its responsibilities do not directly fit with other domain types. The main responsibity is to look after schema and mappings to the table schema.
    More specifically, this type allows:
    
    1) Configuring a WorkspaceMemento with all items from a TableWorkspace.
    2) Checking an arbitary ITableWorkspace to see whether it has a valid schema.
    3) Accessing and Setting MementoItems in a clean and 

    @author Owen Arnold
    @date 12/10/2011

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
namespace MantidQt
{
  namespace CustomInterfaces
  {
    template<typename Memento>
    class DLLExport WorkspaceMementoService
    {
    private:

      template<typename T>
      struct typetraits
      {
        static void cleanUp(T){/*DoNothing*/}
      };

      template<typename T>
      struct typetraits<T*>
      {
        static void cleanUp(T* t)
        {
          delete t;
        }
      };

      template<typename T>
      struct typetraits<T&>
      {
        static void cleanUp(T&){/*DoNothing*/}
      };

      Memento m_memento;

      const int m_logValueStart;

    public:

      WorkspaceMementoService(Memento memento);
      ~WorkspaceMementoService();

      void addAllItems(Mantid::API::ITableWorkspace_sptr, int rowIndex);
      void declareLogItem(std::string name);
      void declareLogItems(Mantid::API::ITableWorkspace_sptr ws, std::vector<Mantid::Kernel::Property*> vecLogData, int rowIndex); //Adds log dat to tws and creates items
      void declareLogItems(Mantid::API::ITableWorkspace_sptr ws, std::vector<std::string> vecLogData, int rowIndex); //Adds log data to tws and creates items
      void addLogItems(Mantid::API::ITableWorkspace_sptr ws, int rowIndex); //Assumes table workspace already exists and is populated
      bool validMementoTableSchema(Mantid::API::ITableWorkspace_sptr);
      void setWorkspaceName(std::string name);
      void setInstrumentName(std::string name);
      void setRunNumber(int runNumber);
      void setShapeXML(std::string shapeXML);
      void setLatticeParameters(double a1, double a2, double a3, double b1, double b2, double b3);
      void setStatus(std::string status);
      void setLogData(std::vector<Mantid::Kernel::Property*> vecLogData);
      void setLogData(std::vector<std::string> vecLogData);

      std::string getInstrumentName();
      std::string getWorkspaceName();
      int getRunNumber();
      std::string getShapeXML();
      double getA1();
      double getA2();
      double getA3();
      double getB1();
      double getB2();
      double getB3();
      std::string getStatus();
      std::vector<AbstractMementoItem_sptr> getLogData();
      
    };
  }
}

#endif