#ifndef MANTID_GEOMETRY_Component_H_
#define MANTID_GEOMETRY_Component_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidKernel/System.h"

#ifdef _WIN32
#pragma warning( disable: 4250 )
#endif

namespace Mantid
{
  namespace Geometry
  {
    /** @class Component
    @brief base class for Geometric component
    @version A
    @author Laurent C Chapon, ISIS RAL
    @date 01/11/2007

    This is the base class for geometric components.
    Geometric component can be placed in a hierarchical
    structure and are defined with respect to a
    parent component. The component position and orientation
    are relatives, i.e. defined with respect to the parent
    component. The orientation is stored as a quaternion.
    Each component has a defined bounding box which at the moment
    is cuboid.

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport Component:public virtual IComponent
    {
    public:
      /// Returns a string representation of the component type
      virtual std::string type() const {return "LogicalComponent";}
      //! Create Empty Component at Origin, with no orientation and null parent
      Component();
      //! Create a named component with a parent component (optional)
      explicit Component(const std::string& name, Component* parent=0);
      //! Create a named component with positioning vector, and parent component (optional)
      Component(const std::string& name, const V3D& position, Component* parent=0);
      //! Create a named component with positioning vector, orientation and parent component
      Component(const std::string& name, const V3D& position, const Quat& rotation, Component* parent=0);
      //Copy constructors
      //! Copy constructor
      Component(const Component&);
      //! Return a clone to the current object
      virtual IComponent* clone() const;
      /// Destructor
      virtual ~Component();
      //! Returns the ComponentID - a unique identifier of the component.
      ComponentID getComponentID()const;
      //! Assign a parent component. Previous parent link is lost
      void setParent(IComponent*);
      //! Return a pointer to the current parent.
      boost::shared_ptr<const IComponent> getParent() const;
      //! Return an array of all ancestors
      std::vector<boost::shared_ptr<const IComponent> > getAncestors() const;
      //! Set the component name
      void setName(const std::string&);
      //! Get the component name
      std::string getName() const;
      //! Set the component position, x, y, z respective to parent (if present) otherwise absolute
      void setPos(double, double, double);
      void setPos(const V3D&);
      //! Set the orientation quaternion relative to parent (if present) otherwise absolute
      void setRot(const Quat&);
      //! Copy the Rotation from another component
      void copyRot(const IComponent&);
      //! Translate the component (vector form). This is relative to parent if present.
      void translate(const V3D&);
      //! Translate the component (x,y,z form). This is relative to parent if present.
      void translate(double, double, double);
      //! Rotate the component. This is relative to parent.
      void rotate(const Quat&);
      //! Rotate the component by an angle in degrees with respect to an axis.
      void rotate(double,const V3D&);
      //! Get the position relative to the parent component (absolute if no parent)
      V3D getRelativePos() const;
      //! Get the position of the component. Tree structure is traverse through the parent chain
      V3D getPos() const;
      //! Get the relative Orientation
      const Quat& getRelativeRot() const;
      //! Get the absolute orientation of the component
      const Quat getRotation() const;
      //! Get the distance to another component
      double getDistance(const IComponent&) const;
      /** @name ParamaterMap access */
      //@{
      // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to resort to
      // one for each type, luckily there won't be too many
      /// Return the parameter names
      virtual std::set<std::string> getParameterNames(bool recursive = true) const;
      /// Returns a boolean indicating whether the parameter exists or not
      bool hasParameter(const std::string & name, bool recursive = true) const;
      /**
      * Get a parameter defined as a double
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of size 0 as this is not a parameterized component
      */
      std::vector<double> getNumberParameter(const std::string& pname, bool recursive = true) const;
      /**
      * Get a parameter defined as a V3D
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of size 0 as this is not a parameterized component
      */
      std::vector<V3D> getPositionParameter(const std::string& pname, bool recursive = true) const;
      /**
      * Get a parameter defined as a Quaternion
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of size 0 as this is not a parameterized component
      */
      std::vector<Quat> getRotationParameter(const std::string& pname, bool recursive = true) const;
      /**
      * Get a parameter defined as a string
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of size 0 as this is not a parameterized component
      */
      std::vector<std::string> getStringParameter(const std::string& pname, bool recursive = true) const;
      //@}

      void printSelf(std::ostream&) const;
    private:
      /// Private, unimplemented copy assignment operator
      Component& operator=(const Component&);

      //! Name of the component
      std::string name;
      //! Position w
      V3D pos;
      //! Orientation
      Quat rot;
      /// Parent component in the tree
      const IComponent* parent;
    };

    DLLExport std::ostream& operator<<(std::ostream&, const Component&);

  } //Namespace Geometry
} //Namespace Mantid

#endif /*MANTID_GEOMETRY_Component_H_*/
