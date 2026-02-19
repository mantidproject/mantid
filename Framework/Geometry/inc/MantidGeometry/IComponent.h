// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#ifndef Q_MOC_RUN
#include <memory>
#endif

namespace Mantid {

namespace Kernel {
class Quat;
}

namespace Geometry {
class IComponent;
class ParameterMap;

/// Define a type for a unique component identifier.
using ComponentID = IComponent *;

/** @class IComponent
@brief base class for Geometric IComponent
@version A
@author Laurent C Chapon, ISIS RAL
@date 01/11/2007

This is the base class for geometric components.
Geometric IComponent can be placed in a hierarchical
structure and are defined with respect to a
parent IComponent. The IComponent position and orientation
are relatives, i.e. defined with respect to the parent
IComponent. The orientation is stored as a Kernel::Quaternion.
Each IComponent has a defined bounding box which at the moment
is cuboid.
*/

class MANTID_GEOMETRY_DLL IComponent {
public:
  /// Returns a string representation of the IComponent type
  virtual std::string type() const { return "LogicalComponent"; }
  //! Return a clone to the current object
  virtual IComponent *clone() const = 0;
  /// Destructor
  virtual ~IComponent() = default;
  //! Returns the ComponentID - a unique identifier of the component.
  virtual ComponentID getComponentID() const = 0;
  //! Returns const pointer to base component if this component is parametrized
  // or pointer to itself if not. Currently is the same as getComponentID bar
  // const cast;
  virtual IComponent const *getBaseComponent() const = 0;
  //! Assign a parent IComponent. Previous parent link is lost
  virtual void setParent(IComponent *) = 0;
  //! Return a pointer to the current parent.
  virtual std::shared_ptr<const IComponent> getParent() const = 0;
  /** Returns the bare pointer to the IComponent parent */
  virtual const IComponent *getBareParent() const = 0;
  //! Return an array of all ancestors, the nearest first
  virtual std::vector<std::shared_ptr<const IComponent>> getAncestors() const = 0;
  //! Set the IComponent name
  virtual void setName(const std::string &) = 0;
  //! Get the IComponent name
  virtual std::string getName() const = 0;
  //! Get the IComponent full path name
  virtual std::string getFullName() const = 0;
  //! Set the IComponent position, x, y, z respective to parent (if present)
  // otherwise absolute
  virtual void setPos(double, double, double) = 0;
  /** Set the position of the component
   *  The position is with respect to the parent component
   */
  virtual void setPos(const Kernel::V3D &) = 0;
  virtual void setSideBySideViewPos(const Kernel::V2D &) = 0;
  //! Set the orientation Kernel::Quaternion relative to parent (if present)
  // otherwise absolute
  virtual void setRot(const Kernel::Quat &) = 0;
  //! Copy the Rotation from another IComponent
  // virtual void copyRot(const IComponent&) = 0;
  //! Translate the IComponent (vector form). This is relative to parent if
  // present.
  virtual void translate(const Kernel::V3D &) = 0;
  //! Translate the IComponent (x,y,z form). This is relative to parent if
  // present.
  virtual void translate(double, double, double) = 0;
  //! Rotate the IComponent. This is relative to parent.
  virtual void rotate(const Kernel::Quat &) = 0;
  //! Rotate the IComponent by an angle in degrees with respect to an axis.
  virtual void rotate(double, const Kernel::V3D &) = 0;
  //! Get the position relative to the parent IComponent (absolute if no parent)
  virtual Kernel::V3D getRelativePos() const = 0;
  //! Get the position of the IComponent. Tree structure is traverse through the
  // parent chain
  virtual Kernel::V3D getPos() const = 0;
  //! Get the position of the IComponent for display on the side by side instrument
  //! view
  virtual std::optional<Kernel::V2D> getSideBySideViewPos() const = 0;
  //! Get the relative Orientation
  virtual Kernel::Quat getRelativeRot() const = 0;
  //! Get the absolute orientation of the IComponent
  virtual Kernel::Quat getRotation() const = 0;
  //! Get the distance to another IComponent
  virtual double getDistance(const IComponent &) const = 0;
  /// Get the bounding box for this component and store it in the given argument
  virtual void getBoundingBox(BoundingBox &boundingBox) const = 0;

  /** Gets the scaling factor of the object for the Object Component.
   * @return a vector with 1 in all 3 directions.
   */
  virtual Kernel::V3D getScaleFactor() const { return Kernel::V3D(1.0, 1.0, 1.0); }

  /** @name ParameterMap access */
  //@{
  /// Return the names of the parameters for this component
  virtual std::set<std::string> getParameterNames(bool recursive = true) const = 0;
  /// return the parameter names and the component they are from
  virtual std::map<std::string, ComponentID> getParameterNamesByComponent() const = 0;
  /// Returns a boolean indicating if the component has the named parameter
  virtual bool hasParameter(const std::string &name, bool recursive = true) const = 0;
  // Hack until proper python export functions are defined
  virtual std::string getParameterType(const std::string &pname, bool recursive = true) const = 0;
  // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to
  // resort to
  // one for each type, luckily there won't be too many
  /// Get a parameter defined as a double
  virtual std::vector<double> getNumberParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as a Kernel::V3D
  virtual std::vector<Kernel::V3D> getPositionParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as a Kernel::Quaternion
  virtual std::vector<Kernel::Quat> getRotationParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as a string
  virtual std::vector<std::string> getStringParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as an integer
  virtual std::vector<int> getIntParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as a boolean
  virtual std::vector<bool> getBoolParameter(const std::string &pname, bool recursive = true) const = 0;
  /// get a string representation of a parameter
  virtual std::string getParameterAsString(const std::string &pname, bool recursive = true) const = 0;
  /// get visibility attribute of a parameter
  virtual bool getParameterVisible(const std::string &pname, bool recursive = true) const = 0;
  //@}
  /** Prints a text representation of itself
   */
  virtual void printSelf(std::ostream &) const = 0;
  //! Returns true if the Component is parametrized (has a parameter map)
  virtual bool isParametrized() const = 0;
  virtual size_t registerContents(class ComponentVisitor &component) const = 0;
};

/// Typedef of a shared pointer to a IComponent
using IComponent_sptr = std::shared_ptr<IComponent>;
/// Typdef of a shared pointer to a const IComponent
using IComponent_const_sptr = std::shared_ptr<const IComponent>;

} // Namespace Geometry

/// This functor is used as the deleter object of a shared_ptr to effectively erase ownership
/// Raw pointers can be passed out as non-owning shared_ptrs that don't delete
class NoDeleting {
public:
  /// Does nothing
  void operator()(void *) {}
  /// Does nothing
  void operator()(const void *) {}
};

} // Namespace Mantid
