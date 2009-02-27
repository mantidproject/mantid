#ifndef MANTID_GEOMETRY_IComponent_H_
#define MANTID_GEOMETRY_IComponent_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Geometry
{
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
    IComponent. The orientation is stored as a quaternion.
    Each IComponent has a defined bounding box which at the moment
    is cuboid.

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class IComponent;
/// Define a type for a unique component identifier.
typedef void* ComponentID;

class DLLExport IComponent
{
public:
  /// Returns a string representation of the IComponent type
  virtual std::string type() const {return "LogicalIComponent";}
  //! Return a clone to the current object
  virtual IComponent* clone() const=0;
  /// Destructor
  virtual ~IComponent(){}
  //! Returns the ComponentID - a unique identifier of the component.
  virtual const ComponentID getComponentID()const = 0;
  //! Assign a parent IComponent. Previous parent link is lost
  virtual void setParent(IComponent*)= 0;
  //! Return a pointer to the current parent.
  virtual const IComponent* getParent() const = 0;
  //! Set the IComponent name
  virtual void setName(const std::string&) = 0;
  //! Get the IComponent name
  virtual std::string getName() const = 0;
  //! Set the IComponent position, x, y, z respective to parent (if present) otherwise absolute
  virtual void setPos(double, double, double) = 0;
  virtual void setPos(const V3D&) = 0;
  //! Set the orientation quaternion relative to parent (if present) otherwise absolute
  virtual void setRot(const Quat&) = 0;
  //! Copy the Rotation from another IComponent
  virtual void copyRot(const IComponent&) = 0;
  //! Translate the IComponent (vector form). This is relative to parent if present.
  virtual void translate(const V3D&) = 0;
  //! Translate the IComponent (x,y,z form). This is relative to parent if present.
  virtual void translate(double, double, double) = 0;
  //! Rotate the IComponent. This is relative to parent.
  virtual void rotate(const Quat&) = 0;
  //! Rotate the IComponent by an angle in degrees with respect to an axis.
  virtual void rotate(double,const V3D&) = 0;
  //! Get the position relative to the parent IComponent (absolute if no parent)
  virtual V3D getRelativePos() const = 0;
  //! Get the position of the IComponent. Tree structure is traverse through the parent chain
  virtual V3D getPos() const = 0;
  //! Get the relative Orientation
  virtual const Quat& getRelativeRot() const = 0;
  //! Get the absolute orientation of the IComponent
  virtual const Quat getRotation() const = 0;
  //! Get the distance to another IComponent
  virtual double getDistance(const IComponent&) const = 0;
  virtual void printSelf(std::ostream&) const = 0;
private:
  /// Private, unimplemented copy assignment operator
  IComponent& operator=(const IComponent&);
};

DLLExport std::ostream& operator<<(std::ostream&, const IComponent&);

} //Namespace Geometry
} //Namespace Mantid

#endif /*MANTID_GEOMETRY_IComponent_H_*/
