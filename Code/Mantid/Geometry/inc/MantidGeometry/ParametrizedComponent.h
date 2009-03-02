#ifndef MANTID_GEOMETRY_PARAMETRIZED_COMPONENT_H_
#define MANTID_GEOMETRY_PARAMETRIZED_COMPONENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/ParameterMap.h"
#include <string>
#include <typeinfo>
#include <vector>

namespace Mantid
{
namespace Geometry
{

/** @class ParametrizedComponent ParametrizedComponent.h Geometry/ParametrizedComponent.h

    ParametrizedComponent is a wrapper for a Component which can modify some of its
    parameters, e.g. position, orientation. Implements IComponent.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 4/12/2008

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
class DLLExport ParametrizedComponent:public virtual IComponent
{
public:
    /// Constructor
    ParametrizedComponent(const IComponent* base, const ParameterMap* map);
    /// Copy Constructor
    ParametrizedComponent(const ParametrizedComponent&);
    ///  destructor
    ~ParametrizedComponent();

     IComponent* clone() const;
     
     //! Returns the ComponentID - a unique identifier of the component.
     const ComponentID getComponentID()const;

    //! Assign a parent IComponent. Previous parent link is lost
     void setParent(IComponent*);

    //! Return a pointer to the current parent.
     boost::shared_ptr<const IComponent> getParent() const;

    //! Set the IComponent name
     void setName(const std::string&);

    //! Get the IComponent name
     std::string getName() const;

    //! Set the IComponent position, x, y, z respective to parent (if present) otherwise absolute
     void setPos(double, double, double);
     void setPos(const V3D&);

    //! Set the orientation quaternion relative to parent (if present) otherwise absolute
     void setRot(const Quat&);

    //! Copy the Rotation from another IComponent
     void copyRot(const IComponent&);

    //! Translate the IComponent (vector form). This is relative to parent if present.
     void translate(const V3D&);

    //! Translate the IComponent (x,y,z form). This is relative to parent if present.
     void translate(double, double, double);

    //! Rotate the IComponent. This is relative to parent.
     void rotate(const Quat&);

    //! Rotate the IComponent by an angle in degrees with respect to an axis.
     void rotate(double,const V3D&);

    //! Get the position relative to the parent IComponent (absolute if no parent)
     V3D getRelativePos() const;

    //! Get the position of the IComponent. Tree structure is traverse through the parent chain
     V3D getPos() const;

    //! Get the relative Orientation
     const Quat& getRelativeRot() const;

    //! Get the absolute orientation of the IComponent
     const Quat getRotation() const;

    //! Get the distance to another IComponent
     double getDistance(const IComponent&) const;
     void printSelf(std::ostream&) const;

     /// Returns the address of the base component
     const IComponent* base()const{return m_base;}

protected:

    /// The base component 
    const IComponent* m_base;

    /// Reference to the map
    const ParameterMap* m_map;

    /// Parent ParametrizedComponent
    //mutable ParametrizedComponent *m_parent;
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_PARAMETRIZED_COMPONENT_H_*/
