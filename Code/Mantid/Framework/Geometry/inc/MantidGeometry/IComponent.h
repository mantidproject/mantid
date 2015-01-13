#ifndef MANTID_GEOMETRY_ICOMPONENT_H_
#define MANTID_GEOMETRY_ICOMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include <string>
#include <vector>
#include <set>
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

namespace Mantid {
namespace Geometry {
//---------------------------------------------------------
// Forward declarations
//---------------------------------------------------------
class IComponent;
class BoundingBox;
class ParameterMap;

/// Define a type for a unique component identifier.
typedef IComponent *ComponentID;

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_GEOMETRY_DLL IComponent {
public:
  /// Returns a string representation of the IComponent type
  virtual std::string type() const { return "LogicalComponent"; }
  //! Return a clone to the current object
  virtual IComponent *clone() const = 0;
  /// Destructor
  virtual ~IComponent() {}
  //! Returns the ComponentID - a unique identifier of the component.
  virtual ComponentID getComponentID() const = 0;
  //! Returns const pointer to base component if this component is parametrized
  // or pointer to itself if not. Currently is the same as getComponentID bar
  // const cast;
  virtual IComponent const *getBaseComponent() const = 0;
  //! Assign a parent IComponent. Previous parent link is lost
  virtual void setParent(IComponent *) = 0;
  //! Return a pointer to the current parent.
  virtual boost::shared_ptr<const IComponent> getParent() const = 0;
  /** Returns the bare pointer to the IComponent parent */
  virtual const IComponent *getBareParent() const = 0;
  //! Return an array of all ancestors, the nearest first
  virtual std::vector<boost::shared_ptr<const IComponent>>
  getAncestors() const = 0;
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
  virtual const Kernel::V3D getRelativePos() const = 0;
  //! Get the position of the IComponent. Tree structure is traverse through the
  // parent chain
  virtual Kernel::V3D getPos() const = 0;
  //! Get the relative Orientation
  virtual const Kernel::Quat &getRelativeRot() const = 0;
  //! Get the absolute orientation of the IComponent
  virtual const Kernel::Quat getRotation() const = 0;
  //! Get the distance to another IComponent
  virtual double getDistance(const IComponent &) const = 0;
  /// Get the bounding box for this component and store it in the given argument
  virtual void getBoundingBox(BoundingBox &boundingBox) const = 0;

  /** Gets the scaling factor of the object for the Object Component.
    * @return a vector with 1 in all 3 directions.
    */
  virtual Kernel::V3D getScaleFactor() const {
    return Kernel::V3D(1.0, 1.0, 1.0);
  }

  /** @name ParameterMap access */
  //@{
  /// Return the names of the parameters for this component
  virtual std::set<std::string>
  getParameterNames(bool recursive = true) const = 0;
  /// return the parameter names and the component they are from
  virtual std::map<std::string, ComponentID>
  getParameterNamesByComponent() const = 0;
  /// Returns a boolean indicating if the component has the named parameter
  virtual bool hasParameter(const std::string &name,
                            bool recursive = true) const = 0;
  // Hack until proper python export functions are defined
  virtual std::string getParameterType(const std::string &pname,
                                       bool recursive = true) const = 0;
  // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to
  // resort to
  // one for each type, luckily there won't be too many
  /// Get a parameter defined as a double
  virtual std::vector<double>
  getNumberParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as a Kernel::V3D
  virtual std::vector<Kernel::V3D>
  getPositionParameter(const std::string &pname,
                       bool recursive = true) const = 0;
  /// Get a parameter defined as a Kernel::Quaternion
  virtual std::vector<Kernel::Quat>
  getRotationParameter(const std::string &pname,
                       bool recursive = true) const = 0;
  /// Get a parameter defined as a string
  virtual std::vector<std::string>
  getStringParameter(const std::string &pname, bool recursive = true) const = 0;
  /// Get a parameter defined as an integer
  virtual std::vector<int> getIntParameter(const std::string &pname,
                                           bool recursive = true) const = 0;
  /// Get a parameter defined as a boolean
  virtual std::vector<bool> getBoolParameter(const std::string &pname,
                                             bool recursive = true) const = 0;
  /// get a string representation of a parameter
  virtual std::string getParameterAsString(const std::string &pname,
                                           bool recursive = true) const = 0;
  //@}
  /** Prints a text representation of itself
  */
  virtual void printSelf(std::ostream &) const = 0;
  //! Returns true if the Component is parametrized (has a parameter map)
  virtual bool isParametrized() const = 0;
};

/// Typedef of a shared pointer to a IComponent
typedef boost::shared_ptr<IComponent> IComponent_sptr;
/// Typdef of a shared pointer to a const IComponent
typedef boost::shared_ptr<const IComponent> IComponent_const_sptr;

/** Prints a text representation
*/
MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &,
                                             const IComponent &);

} // Namespace Geometry

/// An object for constructing a shared_ptr that won't ever delete its pointee
class NoDeleting {
public:
  /// Does nothing
  void operator()(void *) {}
  /// Does nothing
  void operator()(const void *) {}
};

} // Namespace Mantid

#endif // MANTID_GEOMETRY_ICOMPONENT_H_
