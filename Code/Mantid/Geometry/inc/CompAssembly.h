#ifndef COMPONENT_ASSEMBLY_
#define COMPONENT_ASSEMBLY_
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "Component.h"

namespace Mantid
{
  namespace Geometry
  {
    /** @class CompAssembly 
    @brief Class for Assembly of geometric components. 
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    CompAssembly allows Component to be positioned
    in a hierarchical structure in the form of a tree.
    CompAssembly inherit from component.

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: 
    <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    */
    class DLLExport CompAssembly : public Component
    {

    public:
      ///String description of the type of component
      virtual std::string type() const {return "CompAssembly";}
      //! Empty constructor
      CompAssembly();
      //! Constructor with a name and parent reference
      CompAssembly(const std::string&, Component* reference=0);
      //! Copy constructor
      CompAssembly(const CompAssembly&);
      virtual ~CompAssembly();
      //! Make a clone of the present component
      virtual Component* clone() const ;
      //! Return the number of elements in the assembly
      int nelements() const;
      //! Add a component to the assembly
      int add(Component*);
      //! Add a copy (clone) of a component 
      int addCopy(Component*);
      //! Add a copy (clone) of a component and rename it
      int addCopy(Component*,const std::string&);
      //! Get a pointer to the ith component in the assembly
      Component* operator[](int i) const;
      //! Print information about all children
      void printChildren(std::ostream&) const;
      void printTree(std::ostream&) const;
    private:
      ///the group of child components
      std::vector<Component*> group;
    };

    DLLExport std::ostream& operator<<(std::ostream&, const CompAssembly&);
  } //Namespace Geometry
} //Namespace Mantid

#endif
