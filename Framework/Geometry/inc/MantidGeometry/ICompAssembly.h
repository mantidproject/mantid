// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include <deque>
#include <string>
#include <vector>

namespace Mantid {
namespace Geometry {
// Forward declaration
class Track;

/** @class ICompAssembly
@brief Class for Assembly of geometric components.
@version A
@author Laurent C Chapon, ISIS RAL
@date 01/11/2007

CompAssembly allows Components to be positioned
in a hierarchical structure in the form of a tree.
CompAssembly inherits from component.
*/
class MANTID_GEOMETRY_DLL ICompAssembly : public virtual IComponent {
public:
  // Default constructor;
  // Fixes warning C4436 on Windows.
  ICompAssembly() {};
  /// String description of the type of component
  std::string type() const override { return "ICompAssembly"; }
  /// Make a clone of the present component
  IComponent *clone() const override = 0;
  /// Return the number of elements in the assembly
  virtual int nelements() const = 0;
  /// Add a component to the assembly
  virtual int add(IComponent *) = 0;
  /// Add a copy (clone) of a component
  virtual int addCopy(IComponent *) = 0;
  /// Add a copy (clone) of a component and rename it
  virtual int addCopy(IComponent *, const std::string &) = 0;
  /// Get a pointer to the ith component within the assembly. Easier to use than
  /// [] when you have a pointer
  virtual std::shared_ptr<IComponent> getChild(const int i) const = 0;
  /// Returns a pointer to the first component of assembly encountered with the
  /// given name
  virtual std::shared_ptr<const IComponent> getComponentByName(const std::string &cname, int nlevels = 0) const = 0;
  /// Get all children
  virtual void getChildren(std::vector<IComponent_const_sptr> &outVector, bool recursive) const = 0;
  /// Overloaded index operator. Get a pointer to the ith component in the
  /// assembly
  virtual std::shared_ptr<IComponent> operator[](int i) const = 0;
  /// Print information about all children
  virtual void printChildren(std::ostream &) const = 0;
  /** Print information about all the elements in the tree to a stream
   *  Loops through all components in the tree
   *  and call printSelf(os).
   */
  virtual void printTree(std::ostream &) const = 0;

  /// Test the intersection of the ray with the children of the component
  /// assembly
  virtual void testIntersectionWithChildren(Track &testRay, std::deque<IComponent_const_sptr> &searchQueue) const = 0;

protected:
  /// Protected copy constructor
  ICompAssembly(const ICompAssembly &) = default;

private:
  ICompAssembly &operator=(const ICompAssembly &) = delete;
};

/// Shared pointer to a ICompAssembly
using ICompAssembly_sptr = std::shared_ptr<ICompAssembly>;
/// Shared pointer to a const ICompAssembly
using ICompAssembly_const_sptr = std::shared_ptr<const ICompAssembly>;

} // Namespace Geometry
} // Namespace Mantid
