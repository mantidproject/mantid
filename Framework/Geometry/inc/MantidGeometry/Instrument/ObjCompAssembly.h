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
  using comp_it = std::vector<ObjComponent *>::iterator; ///< Iterator type
  using const_comp_it =
      std::vector<ObjComponent *>::const_iterator; ///< Const iterator type
public:
  /// String description of the type of component
  std::string type() const override { return "ObjCompAssembly"; }
  //! Empty constructor
  // ObjCompAssembly();

  ObjCompAssembly(const IComponent *base, const ParameterMap *map);

  //! Constructor with a name and parent reference
  ObjCompAssembly(const std::string &, IComponent *reference = nullptr);
  //! Copy constructor
  ObjCompAssembly(const ObjCompAssembly &);
  ~ObjCompAssembly() override;
  //! Make a clone of the present component
  IComponent *clone() const override;
  //! Return the number of elements in the assembly
  int nelements() const override;
  //! Add a component to the assembly
  int add(IComponent *) override;
  //! Add a copy (clone) of a component
  int addCopy(IComponent *) override;
  //! Add a copy (clone) of a component and rename it
  int addCopy(IComponent *, const std::string &) override;
  //! Get a pointer to the ith component within the assembly. Easier to use than
  //[] when you have a pointer
  boost::shared_ptr<IComponent> getChild(const int i) const override {
    return (*this)[i];
  }
  //! Get all children
  void getChildren(std::vector<IComponent_const_sptr> &outVector,
                   bool recursive) const override;
  //! Returns a pointer to the first component of assembly encountered with the
  // given name
  boost::shared_ptr<const IComponent>
  getComponentByName(const std::string &cname, int nlevels = 0) const override;
  //! Get a pointer to the ith component in the assembly
  boost::shared_ptr<IComponent> operator[](int i) const override;
  //! Print information about all children
  void printChildren(std::ostream &) const override;
  void printTree(std::ostream &) const override;

  Kernel::Quat getRotation() const override;
  Kernel::V3D getPos() const override;

  //! Set the outline of the assembly
  boost::shared_ptr<IObject> createOutline();
  void setOutline(boost::shared_ptr<const IObject> obj);

  /** Test the intersection of the ray with the children of the component
   * assembly  */
  void testIntersectionWithChildren(
      Track & /*testRay*/,
      std::deque<IComponent_const_sptr> & /*searchQueue*/) const override;

  size_t registerContents(
      class Mantid::Geometry::ComponentVisitor &visitor) const override;

private:
  /// Private copy assignment operator
  ObjCompAssembly &operator=(const ICompAssembly &);

  /// the group of child components
  std::vector<ObjComponent *> group;
};

/// Shared pointer to ObjCompAssembly
using ObjCompAssembly_sptr = boost::shared_ptr<ObjCompAssembly>;
/// Shared pointer to ObjCompAssembly (const version)
using ObjCompAssembly_const_sptr = boost::shared_ptr<const ObjCompAssembly>;

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const ObjCompAssembly &);

} // Namespace Geometry
} // Namespace Mantid

#endif /*OBJCOMPONENT_ASSEMBLY_*/
