// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <string>
#include <vector>

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
*/
class MANTID_GEOMETRY_DLL ObjCompAssembly : public virtual ICompAssembly, public virtual ObjComponent {
  using comp_it = std::vector<ObjComponent *>::iterator;             ///< Iterator type
  using const_comp_it = std::vector<ObjComponent *>::const_iterator; ///< Const iterator type
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
  ObjCompAssembly &operator=(const ObjCompAssembly &);
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
  std::shared_ptr<IComponent> getChild(const int i) const override { return (*this)[i]; }
  //! Returns a pointer to the first component of assembly encountered with the
  // given name
  std::shared_ptr<const IComponent> getComponentByName(const std::string &cname, int nlevels = 0) const override;
  //! Get a pointer to the ith component in the assembly
  std::shared_ptr<IComponent> operator[](int i) const override;
  //! Print information about all children
  void printChildren(std::ostream &) const override;
  void printTree(std::ostream &) const override;

  Kernel::Quat getRotation() const override;
  Kernel::V3D getPos() const override;

  //! Set the outline of the assembly
  std::shared_ptr<IObject> createOutline();
  void setOutline(std::shared_ptr<const IObject> obj);

  /** Test the intersection of the ray with the children of the component
   * assembly  */
  void testIntersectionWithChildren(Track & /*testRay*/,
                                    std::deque<IComponent_const_sptr> & /*searchQueue*/) const override;

  size_t registerContents(class Mantid::Geometry::ComponentVisitor &visitor) const override;

protected:
  //! Get all children
  void getChildren(std::vector<IComponent_const_sptr> &outVector, bool recursive) const override;

private:
  /// Private copy assignment operator
  ObjCompAssembly &operator=(const ICompAssembly &);

  /// the group of child components
  std::vector<ObjComponent *> m_group;
};

/// Shared pointer to ObjCompAssembly
using ObjCompAssembly_sptr = std::shared_ptr<ObjCompAssembly>;
/// Shared pointer to ObjCompAssembly (const version)
using ObjCompAssembly_const_sptr = std::shared_ptr<const ObjCompAssembly>;

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &, const ObjCompAssembly &);

} // Namespace Geometry
} // Namespace Mantid
