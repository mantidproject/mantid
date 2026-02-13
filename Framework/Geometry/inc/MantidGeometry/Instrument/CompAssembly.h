// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include <string>
#include <vector>

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace Mantid {
namespace Geometry {

/** @class CompAssembly
    @brief Class for Assembly of geometric components.
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    CompAssembly allows Components to be positioned
    in a hierarchical structure in the form of a tree.
    CompAssembly inherits from component.
*/
class MANTID_GEOMETRY_DLL CompAssembly : public ICompAssembly, public Component {
protected:
  using comp_it = std::vector<IComponent *>::iterator;             ///< Iterator type
  using const_comp_it = std::vector<IComponent *>::const_iterator; ///< Const iterator type

public:
  /// String description of the type of component
  std::string type() const override { return "CompAssembly"; }
  //! Empty constructor
  CompAssembly();
  /// Constructor for parametrized version
  CompAssembly(const IComponent *base, const ParameterMap *map);
  //! Constructor with a name and parent reference
  CompAssembly(const std::string &, IComponent *reference = nullptr);
  //! Copy constructor
  CompAssembly(const CompAssembly &);
  CompAssembly &operator=(const CompAssembly &other);
  ~CompAssembly() override;
  //! Make a clone of the present component
  IComponent *clone() const override;
  //! Return the number of elements in the assembly
  int nelements() const override;
  //! Add a component to the assembly
  int add(IComponent *) override;
  //! Add a copy (clone) of a component

  void addChildren(IComponent *comp);

  int addCopy(IComponent *) override;
  //! Add a copy (clone) of a component and rename it
  int addCopy(IComponent *, const std::string &) override;
  /// Remove a component from the assembly
  int remove(IComponent *);
  //! Get a pointer to the ith component within the assembly. Easier to use than
  //[] when you have a pointer
  std::shared_ptr<IComponent> getChild(const int i) const override;
  //! Get a pointer to the ith component in the assembly
  std::shared_ptr<IComponent> operator[](int i) const override;
  /// Returns a pointer to the first component of assembly encountered with the
  /// given name
  std::shared_ptr<const IComponent> getComponentByName(const std::string &cname, int nlevels = 0) const override;

  Kernel::V3D getPos() const override;

  Kernel::Quat getRotation() const override;

  /// Get the bounding box for this component and store it in the given argument
  void getBoundingBox(BoundingBox &assemblyBox) const override;

  //! Print information about all children
  void printChildren(std::ostream &) const override;
  void printTree(std::ostream &) const override;

  /** Test the intersection of the ray with the children of the component
   * assembly, for InstrumentRayTracer  */
  void testIntersectionWithChildren(Track &testRay, std::deque<IComponent_const_sptr> &searchQueue) const override;

  virtual size_t registerContents(class ComponentVisitor &componentVisitor) const override;

private:
  /// Private copy assignment operator
  CompAssembly &operator=(const ICompAssembly &);

protected:
  //! Returns a vector of all children contained.
  void getChildren(std::vector<IComponent_const_sptr> &outVector, bool recursive) const override;
  /// the group of child components
  std::vector<IComponent *> m_children;

  /// A cached bounding box
  mutable BoundingBox *m_cachedBoundingBox;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &, const CompAssembly &);

} // Namespace Geometry
} // Namespace Mantid
