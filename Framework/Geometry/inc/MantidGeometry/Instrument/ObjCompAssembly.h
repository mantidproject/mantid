#ifndef OBJCOMPONENT_ASSEMBLY_
#define OBJCOMPONENT_ASSEMBLY_
#include <string>
#include <vector>
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace Mantid {
namespace Geometry {
/** @class ObjCompAssembly
    @brief Class for Assembly of geometric components.
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    ObjCompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    ObjCompAssembly inherits from component.

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL ObjCompAssembly : public virtual ICompAssembly,
                                            public virtual ObjComponent {
  typedef std::vector<ObjComponent *>::iterator comp_it; ///< Iterator type
  typedef std::vector<ObjComponent *>::const_iterator
      const_comp_it; ///< Const iterator type
public:
  /// String description of the type of component
  virtual std::string type() const { return "ObjCompAssembly"; }
  //! Empty constructor
  // ObjCompAssembly();

  ObjCompAssembly(const IComponent *base, const ParameterMap *map);

  //! Constructor with a name and parent reference
  ObjCompAssembly(const std::string &, IComponent *reference = 0);
  //! Copy constructor
  ObjCompAssembly(const ObjCompAssembly &);
  virtual ~ObjCompAssembly();
  //! Make a clone of the present component
  virtual IComponent *clone() const;
  //! Return the number of elements in the assembly
  int nelements() const;
  //! Add a component to the assembly
  int add(IComponent *);
  //! Add a copy (clone) of a component
  int addCopy(IComponent *);
  //! Add a copy (clone) of a component and rename it
  int addCopy(IComponent *, const std::string &);
  //! Get a pointer to the ith component within the assembly. Easier to use than
  //[] when you have a pointer
  boost::shared_ptr<IComponent> getChild(const int i) const {
    return (*this)[i];
  }
  //! Get all children
  void getChildren(std::vector<IComponent_const_sptr> &outVector,
                   bool recursive) const;
  //! Returns a pointer to the first component of assembly encountered with the
  // given name
  virtual boost::shared_ptr<const IComponent>
  getComponentByName(const std::string &cname, int nlevels = 0) const;
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const;
  //! Print information about all children
  void printChildren(std::ostream &) const;
  void printTree(std::ostream &) const;

  const Kernel::Quat getRotation() const;
  Kernel::V3D getPos() const;

  //! Set the outline of the assembly
  boost::shared_ptr<Object> createOutline();
  void setOutline(boost::shared_ptr<const Object> obj);

  /** Test the intersection of the ray with the children of the component
   * assembly  */
  virtual void testIntersectionWithChildren(
      Track & /*testRay*/,
      std::deque<IComponent_const_sptr> & /*searchQueue*/) const;

private:
  /// Private copy assignment operator
  ObjCompAssembly &operator=(const ICompAssembly &);

  /// the group of child components
  std::vector<ObjComponent *> group;
};

/// Shared pointer to ObjCompAssembly
typedef boost::shared_ptr<ObjCompAssembly> ObjCompAssembly_sptr;
/// Shared pointer to ObjCompAssembly (const version)
typedef boost::shared_ptr<const ObjCompAssembly> ObjCompAssembly_const_sptr;

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const ObjCompAssembly &);

} // Namespace Geometry
} // Namespace Mantid

#endif /*OBJCOMPONENT_ASSEMBLY_*/
