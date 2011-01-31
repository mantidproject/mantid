#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/Component.h"


namespace Mantid
{
  namespace Geometry
  {

    /** Constructor for a parametrized Component.
    * @param base :: a Component that is the base (un-parametrized) component
    * @param map :: a ParameterMap to parameterize the component
    */
    Component::Component(const IComponent* base,  const ParameterMap * map)
      : m_base(dynamic_cast<const Component*>(base)), m_map(map), m_isParametrized(true)
    {
      if( !m_base )
      {
	throw std::invalid_argument("Component::Component() - Cannot construct a "
				    "parameterized component from an invalid base component.");
      }

    }

    /** Empty constructor
    *  Create a component with null parent
    */
    Component::Component()
      : m_base(NULL), m_map(NULL), m_isParametrized(false), m_name(), m_pos(), m_rot(), m_parent(NULL)
    {
    }

    /** Constructor by value
    *  @param name :: Component name
    *  @param parent :: parent Component (optional)
    */
    Component::Component(const std::string& name, IComponent* parent)
      : m_base(NULL), m_map(NULL), m_isParametrized(false), m_name(name), m_pos(), m_rot(), 
	m_parent(parent)
    {
    }

    /** Constructor by value
    *  @param name :: Component name
    *  @param position :: position
    *  absolute or relative if the parent is defined
    *  @param parent :: parent Component
    */
    Component::Component(const std::string& name, const V3D& position, IComponent* parent)
      : m_base(NULL), m_map(NULL), m_isParametrized(false), m_name(name), m_pos(position), 
	m_rot(), m_parent(parent)
    {
    }

    /** Constructor
    *  @param name :: Component name
    *  @param position :: position (relative to parent, if present)
    *  @param rotation :: orientation quaternion (relative to parent, if present)
    *  @param parent :: parent Component (optional)
    */
    Component::Component(const std::string& name, const V3D& position, const Quat& rotation, IComponent* parent)
      : m_base(NULL), m_map(NULL), m_isParametrized(false), m_name(name),m_pos(position),
	m_rot(rotation),m_parent(parent)
    {
    }

    /// Destructor
    Component::~Component()
    {
    }



    //------------------------------------------------------------------------------------------------
    /** Return true if the Component is, in fact, parametrized
    *  (that is - it has a valid parameter map)
    */
    bool Component::isParametrized() const
    {
      return m_isParametrized;
    }

    /** Clone method
    *  Make a copy of the Component
    *  @return new(*this)
    */
    IComponent* Component::clone() const
    {
      return new Component(*this);
    }

    /**  Get the component's ID (pointer address)
    *   @return ID
    */
    ComponentID Component::getComponentID()const
    {
      if (m_isParametrized)
        return ComponentID(m_base);
      else
        return ComponentID(this);
    }

    //-------------------------------------------------------------------------------
    /** Set the parent. Previous parenting is lost.
    *  @param comp :: the parent Component
    */
    void Component::setParent(IComponent* comp)
    {
      m_parent = comp;
    }


    //--------------------------------------------------------------------------------------------
    /** Get a pointer to the parent.
    *  @return this.parent
    */
    boost::shared_ptr<const IComponent> Component::getParent() const
    {
      if (this->m_isParametrized)
      {
        boost::shared_ptr<const IComponent> parent = m_base->getParent();
        return ParComponentFactory::createParComponent(parent,m_map);
      }
      else
        return boost::shared_ptr<const IComponent>(m_parent, NoDeleting());
    }

    //--------------------------------------------------------------------------------------------
    /** Returns an array of all ancestors of this component,
    *  starting with the direct parent and moving up
    *  @return An array of pointers to ancestor components
    */
    std::vector<boost::shared_ptr<const IComponent> > Component::getAncestors() const
    {
      std::vector<boost::shared_ptr<const IComponent> > ancs;

      if (m_isParametrized)
      {
        boost::shared_ptr<const IComponent> current = this->getParent();
        while (current)
        {
          ancs.push_back( current );
          current = current->getParent();
        }
      }
      else
      {
        boost::shared_ptr<const IComponent> current = this->getParent();
        while (current)
        {
          ancs.push_back( current );
          current = current->getParent();
        }
      }

      return ancs;
    }

    //--------------------------------------------------------------------------------------------
    /** Set the name of the Component (currently does nothing)
    *  @param s :: name string
    */
    void Component::setName(const std::string& s)
    {
      if (!m_isParametrized)
        this->m_name = s;
      else
        throw Kernel::Exception::NotImplementedError("Component::setName (for Parametrized Component)");
    }

    //--------------------------------------------------------------------------------------------
    /** Get the name of the Component
    *  @return this.name
    */
    std::string Component::getName() const
    {
      if (m_isParametrized)
        return m_base->getName();
      else
        return m_name;
    }

    /** Set the position of the Component
    *  The position is with respect to the parent Component
    *  @param x :: x position
    *  @param y :: y position
    *   @param z :: z position
    */
    void Component::setPos(double x, double y, double z)
    {
      if (!m_isParametrized)
        m_pos = V3D(x,y,z);
      else
        throw Kernel::Exception::NotImplementedError("Component::setPos (for Parametrized Component)");
    }

    /** Set the position of the Component
    *  The position is with respect to the parent Component
    *  @param v :: vector position
    */
    void Component::setPos(const V3D& v)
    {
      if (!m_isParametrized)
        m_pos = v;
      else
        throw Kernel::Exception::NotImplementedError("Component::setPos (for Parametrized Component)");
    }

    /** Set the orientation of the Component
    *  The position is with respect to the parent Component
    *  @param q :: rotation quaternion
    */
    void Component::setRot(const Quat& q)
    {
      if (!m_isParametrized)
        m_rot = q;
      else
        throw Kernel::Exception::NotImplementedError("Component::setRot (for Parametrized Component)");
    }


    /** Translate the Component relative to the parent Component
    *  @param x :: translation on the x-axis
    *   @param y :: translation on the y-axis
    *  @param z :: translation on the z-axis
    */
    void Component::translate(double x, double y, double z)
    {
      if (!m_isParametrized)
      {
        m_pos[0]+=x;
        m_pos[1]+=y;
        m_pos[2]+=z;
      }
      else
        throw Kernel::Exception::NotImplementedError("Component::translate (for Parametrized Component)");
    }

    /** Translate the Component relative to the parent Component
    *  @param v :: translation vector
    */
    void Component::translate(const V3D& v)
    {
      if (!m_isParametrized)
        m_pos+=v;
      else
        throw Kernel::Exception::NotImplementedError("Component::translate (for Parametrized Component)");
    }

    /** Rotate the Component relative to the parent Component
    *  @param r :: translation vector
    */
    void Component::rotate(const Quat& r)
    {
      if (!m_isParametrized)
        m_rot=m_rot*r;
      else
        throw Kernel::Exception::NotImplementedError("Component::rotate (for Parametrized Component)");
    }

    /** Rotate the Component by an angle in degrees with respect to an axis.
    * @param angle :: the number of degrees to rotate
    * @param axis :: The Vector to rotate around
    */
    void Component::rotate(double angle, const V3D& axis)
    {
      (void) angle; //Avoid compiler warning
      (void) axis; //Avoid compiler warning
      throw Kernel::Exception::NotImplementedError("Rotate(double angle, const V3D& axis) has not been implemented");
    }

    /** Gets the position relative to the parent
    * @returns A vector of the relative position
    */
    const V3D & Component::getRelativePos() const
    {
      if( m_isParametrized )
      {
	if( m_map->contains(m_base, "pos") )
	{
	  return m_map->get(m_base, "pos")->value<V3D>();
	}
	else return m_base->m_pos;
      }
      else return m_pos;
    }

    /** Get ScaleFactor of detector
    * @returns A vector of the scale factors (1,1,1) if not set
    */
    V3D Component::getScaleFactorP() const
    {
      if (m_isParametrized)
      {
        Parameter_sptr par = m_map->get(m_base,"sca");
        if (par)
        {
          return par->value<V3D>();
        }
      }
      return V3D(1,1,1);
    }

    /** Gets the absolute position of the Component
    * @returns A vector of the absolute position
    */
    V3D Component::getPos() const
    {
      if (this->m_isParametrized)
      {
        // Avoid instantiation of the parent's parameterized object if possible
	const IComponent * baseParent = m_base->m_parent;
        if ( !baseParent )
        {
          return getRelativePos();
        }
        else
        {
	  // Avoid instantiation of parent shared pointer if we can
	  V3D absPos = getRelativePos();
	  //get the parent rotation, try to get it from the cache first to avoid instantiaing the class
	  Quat parentRot;
	  V3D parentPos;
	  if( !(m_map->getCachedLocation(baseParent, parentPos) && 
		m_map->getCachedRotation(baseParent, parentRot)) )
	  {
	    // Couldn't get them from the cache, so I have to instantiate the class
	    boost::shared_ptr<const IComponent> parParent = getParent();
	    if( parParent )
	    {
	      parentRot = parParent->getRotation();
	      parentPos = parParent->getPos();
	    }
	  }
	  parentRot.rotate(absPos);
	  absPos += parentPos;
	  return absPos;
	}
      }
      else
      {
        if (!m_parent)
        {
          return m_pos;
        }
        else
        {
          V3D absPos(m_pos);
          m_parent->getRotation().rotate(absPos);
          return absPos + m_parent->getPos();
        }
      }
    }

    /** Gets the rotation relative to the parent
    * @returns A quaternion of the relative rotation
    */
    const Quat& Component::getRelativeRot() const
    {
      if( m_isParametrized )
      {
	if( m_map->contains(m_base, "rot") ) 
	{
	  return m_map->get(m_base, "rot")->value<Quat>();
	}
	return m_base->m_rot;
      }
      else return m_rot;
    }

    /** Returns the absolute rotation of the Component
    *  @return A quaternion representing the total rotation
    */
    const Quat Component::getRotation() const
    {
      if (m_isParametrized)
      {
        // Avoid instantiation of the parent's parameterized object if possible
	const IComponent * baseParent = m_base->m_parent;
        if ( !baseParent )
        {
          return getRelativeRot();
        }
        else
        {
	  Quat parentRot;
 	  if( !m_map->getCachedRotation(baseParent, parentRot) )
	  {
	    // Get the parent's rotation
	    boost::shared_ptr<const IComponent> parParent = getParent();
	    if( parParent )
	    {
	      parentRot = parParent->getRotation();
	    }
	  }
	  return parentRot*getRelativeRot();
	}
      }
      else
      {
        // Not parametrized
        if (!m_parent)
          return m_rot;
        else
          return m_parent->getRotation()*m_rot;
      }
    }


    /** Gets the distance between two Components
    *  @param comp :: The Component to measure against
    *  @returns The distance
    */
    double Component::getDistance(const IComponent& comp) const
    {
      return getPos().distance(comp.getPos());
    }

    /**
    * Get the bounding box for this component. It is no shape so gives an empty box.
    * @param boundingBox :: [Out] The resulting bounding box is stored here.
    */
    void Component::getBoundingBox(BoundingBox & boundingBox) const
    {
      boundingBox = BoundingBox();
    }

    /**
    * Get the names of the parameters for this component.
    * @param recursive :: If true, the parameters for all of the parent components are also included
    * @returns A set of strings giving the parameter names for this component
    */
    std::set<std::string> Component::getParameterNames(bool recursive) const
    {
      if (!m_isParametrized) return std::set<std::string>();

      std::set<std::string> names = m_map->names(this);
      if( recursive )
      {
        //Walk up the tree and find the parameters attached to the parent components
        boost::shared_ptr<const IComponent> parent = getParent();
        if( parent )
        {
          std::set<std::string> parentNames = parent->getParameterNames(true);
          names.insert(parentNames.begin(), parentNames.end());
        }
      }
      return names;
    }

    /**
    * Returns a boolean indicating if the component has the named parameter
    * @param name :: The name of the parameter
    * @param recursive :: If true the parent components will also be searched (Default: true)
    * @returns A boolean indicating if the search was successful or not.
    */
    bool Component::hasParameter(const std::string & name, bool recursive) const
    {
      if (!m_isParametrized) return false;

      bool match_found(false);
      if( m_map->contains(this, name) )
      {
        match_found = true;
      }
      else if( recursive )
      {
        boost::shared_ptr<const IComponent> parent = getParent();
        if( parent )
        {
          match_found = parent->hasParameter(name, true);
        }
        else
        {
          match_found = false;
        }
      }
      else
      {
        match_found = false;
      }
      return match_found;
    }


    /** Prints a text representation of itself
    * @param os :: The ouput stream to write to
    */
    void Component::printSelf(std::ostream& os) const
    {
      boost::shared_ptr<const IComponent> parent = getParent();
      os << "Name : " << getName() << std::endl;
      os << "Type: " << this->type() << std::endl;
      if (parent)
        os << "Parent: " << parent->getName() << std::endl;
      else
        os << "Parent: None" << std::endl;

      os << "Position : " << getPos() << std::endl;
      os << "Orientation :" << getRelativeRot() << std::endl;
    }

    /** Prints a text representation
    * @param os :: The ouput stream to write to
    * @param comp :: The Component to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const Component& comp)
    {
      comp.printSelf(os);
      return os;
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------

    /**
     * Swap the current references to the un-parameterized component and 
     * parameter map for new ones
     * @param base A pointer to the new un-parameterized component
     * @param pmap A pointer to the new parameter map
     */
    void Component::swap(const Component* base, const ParameterMap * pmap)
    {
      m_base = base;
      m_map = pmap;
    }

  } // Namespace Geometry

} // Namespace Mantid

