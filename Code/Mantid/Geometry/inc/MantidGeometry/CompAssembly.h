#ifndef COMPONENT_ASSEMBLY_
#define COMPONENT_ASSEMBLY_
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/ICompAssembly.h"

#ifdef _WIN32
  #pragma warning( disable: 4250 )
#endif

namespace Mantid
{
namespace Geometry
{
/** @class CompAssembly 
    @brief Class for Assembly of geometric components. 
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    CompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    CompAssembly inherits from component.

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CompAssembly : public ICompAssembly, public Component
{
    typedef std::vector< IComponent* >::iterator comp_it;
    typedef std::vector< IComponent* >::const_iterator const_comp_it;
public:
  ///String description of the type of component
  virtual std::string type() const { return "CompAssembly";}
  //! Empty constructor
  CompAssembly();
  //! Constructor with a name and parent reference
  CompAssembly(const std::string&, Component* reference=0);
  //! Copy constructor
  CompAssembly(const CompAssembly&);
  virtual ~CompAssembly();
  //! Make a clone of the present component
  virtual IComponent* clone() const;
  //! Return the number of elements in the assembly
  int nelements() const;
  //! Add a component to the assembly
  int add(IComponent*);
  //! Add a copy (clone) of a component 
  int addCopy(IComponent*);
  //! Add a copy (clone) of a component and rename it
  int addCopy(IComponent*, const std::string&);
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const;
  //! Print information about all children
  void printChildren(std::ostream&) const;
  void printTree(std::ostream&) const;
private:
  /// Private copy assignment operator
  CompAssembly& operator=(const ICompAssembly&);

  ///the group of child components
  std::vector< IComponent* > group;
};

DLLExport std::ostream& operator<<(std::ostream&, const CompAssembly&);

} //Namespace Geometry
} //Namespace Mantid

#endif
