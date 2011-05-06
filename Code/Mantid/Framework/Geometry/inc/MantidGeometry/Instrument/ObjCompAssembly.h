#ifndef OBJCOMPONENT_ASSEMBLY_
#define OBJCOMPONENT_ASSEMBLY_
#include <string> 
#include <vector>
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

#ifdef _WIN32
  #pragma warning( disable: 4250 )
#endif

namespace Mantid
{
namespace Geometry
{
/** @class ObjCompAssembly 
    @brief Class for Assembly of geometric components. 
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    ObjCompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    ObjCompAssembly inherits from component.

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ObjCompAssembly : public virtual ICompAssembly, public virtual ObjComponent
{
    typedef std::vector< ObjComponent* >::iterator comp_it;///< Iterator type
    typedef std::vector< ObjComponent* >::const_iterator const_comp_it;///< Const iterator type
public:
  ///String description of the type of component
  virtual std::string type() const { return "ObjCompAssembly";}
  //! Empty constructor
  //ObjCompAssembly();

  ObjCompAssembly(const IComponent* base, const ParameterMap * map);

  //! Constructor with a name and parent reference
  ObjCompAssembly(const std::string&, Component* reference=0);
  //! Copy constructor
  ObjCompAssembly(const ObjCompAssembly&);
  virtual ~ObjCompAssembly();
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
  //! Get a pointer to the ith component within the assembly. Easier to use than [] when you have a pointer
  boost::shared_ptr<IComponent> getChild(const int i) const{return (*this)[i];}
  //! Get all children
  void getChildren(std::vector<boost::shared_ptr<IComponent> > & outVector, bool recursive) const;
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const;
  //! Print information about all children
  void printChildren(std::ostream&) const;
  void printTree(std::ostream&) const;

  const Quat getRotation() const;
  V3D getPos() const;

  //! Set the outline of the assembly
  boost::shared_ptr<Object> createOutline();
  void setOutline(boost::shared_ptr<const Object> obj);

  /** Test the intersection of the ray with the children of the component assembly  */
  virtual void testIntersectionWithChildren(Track & /*testRay*/, std::deque<IComponent_sptr> & /*searchQueue*/) const
  { throw std::runtime_error("Not implemented."); }


private:
  /// Private copy assignment operator
  ObjCompAssembly& operator=(const ICompAssembly&);

  ///the group of child components
  std::vector< ObjComponent* > group;
};

DLLExport std::ostream& operator<<(std::ostream&, const ObjCompAssembly&);

} //Namespace Geometry
} //Namespace Mantid

#endif /*OBJCOMPONENT_ASSEMBLY_*/
