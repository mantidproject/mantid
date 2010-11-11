#ifndef MANTID_GEOMETRY_Component_H_
#define MANTID_GEOMETRY_Component_H_


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include <string>
#include <typeinfo>
#include <vector>

namespace Mantid
{
  namespace Geometry
  {

    //----------------------------------------------------------------------
    // Forward declarations
    //----------------------------------------------------------------------
    class V3D;
    class Quat;

    /** @class Component Component.h Geometry/Component.h

    Component is a wrapper for a Component which can modify some of its
    parameters, e.g. position, orientation. Implements IComponent.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 4/12/2008

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
      /// Constructor for parametrized component
      Component(const IComponent* base, ParameterMap_const_sptr map);

      //! Create Empty Component at Origin, with no orientation and null parent
      Component();
      //! Create a named component with a parent component (optional)
      explicit Component(const std::string& name, IComponent* parent=0);
      //! Create a named component with positioning vector, and parent component (optional)
      Component(const std::string& name, const V3D& position, IComponent* parent=0);
      //! Create a named component with positioning vector, orientation and parent component
      Component(const std::string& name, const V3D& position, const Quat& rotation, IComponent* parent=0);

      /// Copy Constructor
      Component(const Component&);
      ///  destructor
      ~Component();

      IComponent* clone() const;

      bool isParametrized() const;

      //! Returns the ComponentID - a unique identifier of the component.
      ComponentID getComponentID()const;

      //! Assign a parent IComponent. Previous parent link is lost
      void setParent(IComponent*);

      //! Return a pointer to the current parent.
      boost::shared_ptr<const IComponent> getParent() const;
      //! Return an array of all ancestors
      std::vector<boost::shared_ptr<const IComponent> > getAncestors() const;

      //! Set the IComponent name
      void setName(const std::string&);

      //! Get the IComponent name
      std::string getName() const;

      //! Set the IComponent position, x, y, z respective to parent (if present) otherwise absolute
      void setPos(double, double, double);
      void setPos(const V3D&);

      //! Set the orientation quaternion relative to parent (if present) otherwise absolute
      void setRot(const Quat&);


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
      virtual V3D getPos() const;

      //! Get the relative Orientation
      const Quat& getRelativeRot() const;

      //! Get the absolute orientation of the IComponent
      virtual const Quat getRotation() const;

      //! Get the distance to another IComponent
      double getDistance(const IComponent&) const;

      /// Get the bounding box for this component and store it in the given argument
      virtual void getBoundingBox(BoundingBox& boundingBox) const;

      /** @name ParameterMap access */
      //@{
      // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to resort to
      // one for each type, luckily there won't be too many
      /// Return the parameter names
      virtual std::set<std::string> getParameterNames(bool recursive = true) const;
      /// Returns a boolean indicating if the component has the named parameter
      virtual bool hasParameter(const std::string & name, bool recursive = true) const;

      /**
      * Get a parameter defined as a double
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of values
      */
      std::vector<double> getNumberParameter(const std::string& pname, bool recursive = true) const
      {
        return getParameter<double>(pname, recursive);
      }

      /**
      * Get a parameter defined as a V3D
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of values
      */
      std::vector<V3D> getPositionParameter(const std::string& pname, bool recursive = true) const
      {
        return getParameter<V3D>(pname, recursive);
      }

      /**
      * Get a parameter defined as a Quaternion
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of values
      */
      std::vector<Quat> getRotationParameter(const std::string& pname, bool recursive = true) const
      {
        return getParameter<Quat>(pname, recursive);
      }

      /**
      * Get a parameter defined as a string
      * @param pname The name of the parameter
      * @param recursive If true the search will walk up through the parent components
      * @returns A list of values
      */
      std::vector<std::string> getStringParameter(const std::string& pname, bool recursive = true) const
      {
        return getParameter<std::string>(pname, recursive);
      }
      //@}

      void printSelf(std::ostream&) const;

      /// Returns the address of the base component
      const IComponent* base()const { return m_base;}

      /// Returns the ScaleFactor
      virtual V3D getScaleFactorP() const;

    protected:

      /// The base component - this is the unmodifed component (without the parameters)
      const IComponent* m_base;

      /// Reference to the map containing the parameters
      ParameterMap_const_sptr m_map;

      /// Flag to determine if component is parameterized
      const bool m_isParametrized;

      //! Name of the component
      std::string name;

      //! Position w
      V3D pos;

      //! Orientation
      Quat rot;

      /// Parent component in the tree
      const IComponent* parent;



    private:
      /// Private, unimplemented copy assignment operator
      Component& operator=(const Component&);

      /**
      *  Get a parameter from the parameter map
      * @param p_name The name of the parameter
      * @param recursive If true then the lookup will walk up the tree if this component does not have parameter
      * @return A list of size 0 or 1 containing the parameter value or
      * nothing if it does not exist
      */
      template <class TYPE>
      std::vector<TYPE> getParameter(const std::string & p_name, bool recursive) const
      {
        if (m_isParametrized)
        {
          Parameter_sptr param = Parameter_sptr(); //Null shared pointer
          if( recursive )
          {
            param = m_map->getRecursive(this, p_name);
          }
          else
          {
            param = m_map->get(this, p_name);
          }
          if( param != Parameter_sptr() )
          {
            return std::vector<TYPE>(1, param->value<TYPE>());
          }
          else
          {
            return std::vector<TYPE>(0);
          }
        }
        else
        {
          //Not parametrized = return empty vector
          return std::vector<TYPE>(0);
        }
      }
    };

  } // namespace Geometry
} // namespace Mantid



#endif /*MANTID_GEOMETRY_Component_H_*/
