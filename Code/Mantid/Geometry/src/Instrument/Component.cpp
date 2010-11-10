#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/Component.h"


namespace Mantid
{
  namespace Geometry
  {

  /** Constructor for a parametrized Component.
   * @param base a Component that is the base (un-parametrized) component
   * @param
   *
   */
    Component::Component(const IComponent* base,  ParameterMap_const_sptr map)
      : m_base(base), m_map(map), m_isParametrized(true)
    {
      const Component * comp_base = dynamic_cast<const Component *>(base);
      if (!comp_base)
        throw std::invalid_argument("Component: parametrized constructor called with a pure virtual IComponent base!");
    }

    /// Copy constructor
    Component::Component(const Component& comp)
      :m_base(comp.m_base),m_map(comp.m_map),m_isParametrized(comp.m_isParametrized)
    {
	  name=comp.name;
	  parent=comp.parent;
	  pos=comp.pos;
	  rot=comp.rot;
    }


    /*! Empty constructor
     *  Create a component with null parent
     */
    Component::Component()
      : m_map(), m_isParametrized(false), name(), pos(), rot(), parent(NULL)
    {
    }

    /*! Constructor by value
     *  @param name :: Component name
     *  @param parent :: parent Component (optional)
     */
    Component::Component(const std::string& name, IComponent* parent)
      : m_map(), m_isParametrized(false), name(name), pos(), rot(), parent(parent)
    {
    }

    /*! Constructor by value
     *  @param name :: Component name
     *  @param position :: position
     *  absolute or relative if the parent is defined
     *  @param parent :: parent Component
     */
    Component::Component(const std::string& name, const V3D& position, IComponent* parent)
      : m_map(), m_isParametrized(false), name(name), pos(position), rot(), parent(parent)
    {
    }

    /*! Constructor
     *  @param name :: Component name
     *  @param position :: position (relative to parent, if present)
     *  @param rotation :: orientation quaternion (relative to parent, if present)
     *  @param parent :: parent Component (optional)
     */
    Component::Component(const std::string& name, const V3D& position, const Quat& rotation, IComponent* parent)
      : m_map(), m_isParametrized(false), name(name),pos(position),rot(rotation),parent(parent)
    {
    }

    /// Destructor
    Component::~Component()
    {
    	//std::cout << "Component '" << this->name << "' being deleted.\n";
      //if (m_map)        delete m_map;
    }



    //------------------------------------------------------------------------------------------------
    /** Return true if the Component is, in fact, parametrized
     *  (that is - it has a valid parameter map)
     */
    bool Component::isParametrized() const
    {
      return m_isParametrized;
    }

    /*! Clone method
    *  Make a copy of the Component
    *  @return new(*this)
    */
    IComponent* Component::clone() const
    {
      return new Component(*this);
    }

    /*!  Get the component's ID (pointer address)
    *   @return ID
    */
    ComponentID Component::getComponentID()const
    {
      if (isParametrized())
        return ComponentID(m_base);
      else
        return ComponentID(this);
    }

    /*! Set the parent. Previous parenting is lost.
    *  @param comp :: the parent Component
    */
    void Component::setParent(IComponent* comp)
    {
      parent=comp;
    }

    /*! Get a pointer to the parent.
    *  @return this.parent
    */
    boost::shared_ptr<const IComponent> Component::getParent() const
    {
      if (isParametrized())
      {
        boost::shared_ptr<const IComponent> parent = m_base->getParent();
        return ParComponentFactory::create(parent,m_map);
      }
      else
        return boost::shared_ptr<const IComponent>(parent, NoDeleting());
    }

    /*! Returns an array of all ancestors of this component,
     *  starting with the direct parent and moving up
     *  @return An array of pointers to ancestor components
     */
    std::vector<boost::shared_ptr<const IComponent> > Component::getAncestors() const
    {
        std::vector<boost::shared_ptr<const IComponent> > ancs;
        boost::shared_ptr<const IComponent> current = this->getParent();
        while (current)
        {
          ancs.push_back( ParComponentFactory::create(current,m_map) );
          current = current->getParent();
        }
        return ancs;
    }

    /*! Set the name of the Component (currently does nothing)
    *  @param s :: name string
    */
    void Component::setName(const std::string& s)
    {
      if (!isParametrized())
        this->name = s;
      else
        throw Kernel::Exception::NotImplementedError("Component::setName (for Parametrized Component)");
    }

    /*! Get the name of the Component
    *  @return this.name
    */
    std::string Component::getName() const
    {
      if (isParametrized())
        return m_base->getName();
      else
        return name;
    }

    /*! Set the position of the Component
    *  The position is with respect to the parent Component
    *  @param x :: x position
    *  @param y :: y position
    *   @param z :: z position
    */
    void Component::setPos(double x, double y, double z)
    {
      if (!isParametrized())
        pos(x,y,z);
      else
        throw Kernel::Exception::NotImplementedError("Component::setPos (for Parametrized Component)");
    }

    /*! Set the position of the Component
    *  The position is with respect to the parent Component
    *  @param v :: vector position
    */
    void Component::setPos(const V3D& v)
    {
      if (!isParametrized())
        pos = v;
      else
        throw Kernel::Exception::NotImplementedError("Component::setPos (for Parametrized Component)");
    }

    /*! Set the orientation of the Component
    *  The position is with respect to the parent Component
    *  @param q :: rotation quaternion
    */
    void Component::setRot(const Quat& q)
    {
      if (!isParametrized())
        rot = q;
      else
        throw Kernel::Exception::NotImplementedError("Component::setRot (for Parametrized Component)");
    }


    /*! Translate the Component relative to the parent Component
    *  @param x :: translation on the x-axis
    *   @param y :: translation on the y-axis
    *  @param z :: translation on the z-axis
    */
    void Component::translate(double x, double y, double z)
    {
      if (!isParametrized())
      {
        pos[0]+=x;
        pos[1]+=y;
        pos[2]+=z;
      }
      else
        throw Kernel::Exception::NotImplementedError("Component::translate (for Parametrized Component)");
    }

    /*! Translate the Component relative to the parent Component
    *  @param v :: translation vector
    */
    void Component::translate(const V3D& v)
    {
      if (!isParametrized())
        pos+=v;
      else
        throw Kernel::Exception::NotImplementedError("Component::translate (for Parametrized Component)");
    }

    /*! Rotate the Component relative to the parent Component
    *  @param r :: translation vector
    */
    void Component::rotate(const Quat& r)
    {
      if (!isParametrized())
        rot=rot*r;
      else
        throw Kernel::Exception::NotImplementedError("Component::rotate (for Parametrized Component)");
    }

    /*! Rotate the Component by an angle in degrees with respect to an axis.
    * @param angle the number of degrees to rotate
    * @param axis The Vector to rotate around
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
    V3D Component::getRelativePos() const
    {
      if (!isParametrized())
        return pos;
      else
      {
        Parameter_sptr par = m_map->get(m_base,"pos");
        if (par)
        {
          return par->value<V3D>();
        }
        return m_base->getRelativePos();
      }
    }

    /** Get ScaleFactor of detector
    * @returns A vector of the scale factors (1,1,1) if not set
    */
    V3D Component::getScaleFactorP() const
    {
      if (isParametrized())
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
      if (isParametrized())
      {
        boost::shared_ptr<const IComponent> parent = getParent();
        if (!parent)
        {
          return getRelativePos();
        }
        else
        {
          V3D temp(getRelativePos());
          parent->getRotation().rotate(temp);
          temp+=parent->getPos();
          return temp;
        }
      }
      else
      {
        if (!parent)
        {
          return pos;
        }
        else
        {
          V3D temp(pos);
          parent->getRotation().rotate(temp);
          temp+=parent->getPos();
          return temp;
        }
      }
    }

    /** Gets the rotation relative to the parent
    * @returns A quaternion of the relative rotation
    */
    const Quat& Component::getRelativeRot() const
    {
      if (isParametrized())
      {
        Parameter_sptr par = m_map->get(m_base,"rot");
        if (par)
          return par->value<Quat>();
        else
          return m_base->getRelativeRot();
      }
      else
        return rot;
    }

    /** Returns the absolute rotation of the Component
    *  @return A quaternion representing the total rotation
    */
    const Quat Component::getRotation() const
    {
      boost::shared_ptr<const IComponent> parent = getParent();
      if (!parent)
      {
        return getRelativeRot();
      }
      else
      {
        return parent->getRotation()*getRelativeRot();
      }
    }

    /** Gets the distance between two Components
    *  @param comp The Component to measure against
    *  @returns The distance
    */
    double Component::getDistance(const IComponent& comp) const
    {
      return getPos().distance(comp.getPos());
    }

    /**
    * Get the bounding box for this component. It is no shape so gives an empty box.
    * @param boundingBox [Out] The resulting bounding box is stored here.
    */
    void Component::getBoundingBox(BoundingBox & boundingBox) const
    {
      boundingBox = BoundingBox();
    }

    /**
    * Get the names of the parameters for this component.
    * @param recursive If true, the parameters for all of the parent components are also included
    * @returns A set of strings giving the parameter names for this component
    */
    std::set<std::string> Component::getParameterNames(bool recursive) const
    {
      if (!isParametrized()) return std::set<std::string>();

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
     * @param name The name of the parameter
     * @param recursive If true the parent components will also be searched (Default: true)
     * @returns A boolean indicating if the search was successful or not.
     */
    bool Component::hasParameter(const std::string & name, bool recursive) const
    {
      if (!isParametrized()) return false;

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
    * @param os The ouput stream to write to
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
    * @param os The ouput stream to write to
    * @param comp The Component to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const Component& comp)
    {
      comp.printSelf(os);
      return os;
    }

  } // Namespace Geometry

} // Namespace Mantid

