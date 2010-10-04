#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"


namespace Mantid
{
  namespace Geometry
  {

    ParametrizedComponent::ParametrizedComponent(const IComponent* base,  const ParameterMap& map)
      :m_base(base),m_map(map)
    {
    }

    ParametrizedComponent::ParametrizedComponent(const ParametrizedComponent& comp)
      :m_base(comp.m_base),m_map(comp.m_map)
    {
    }

    ParametrizedComponent::~ParametrizedComponent()
    {
    }

    /*! Clone method
    *  Make a copy of the ParametrizedComponent
    *  @return new(*this)
    */
    IComponent* ParametrizedComponent::clone() const
    {
      return new ParametrizedComponent(*this);
    }

    /*!  Get the component's ID
    *   @return ID
    */
    ComponentID ParametrizedComponent::getComponentID()const
    {
      return ComponentID(m_base);
    }

    /*! Set the parent. Previous parenting is lost.
    *  @param comp :: the parent ParametrizedComponent
    */
    void ParametrizedComponent::setParent(IComponent* comp)
    {
      (void) comp; //Avoid compiler warnings
      //  parent=comp;
    }

    /*! Get a pointer to the parent.
    *  @return this.parent
    */
    boost::shared_ptr<const IComponent> ParametrizedComponent::getParent() const
    {
      boost::shared_ptr<const IComponent> parent = m_base->getParent();
      return ParComponentFactory::create(parent,m_map);
    }

    /*! Returns an array of all ancestors of this component,
     *  starting with the direct parent and moving up
     *  @return An array of pointers to ancestor components
     */
    std::vector<boost::shared_ptr<const IComponent> > ParametrizedComponent::getAncestors() const
    {
      std::vector<boost::shared_ptr<const IComponent> > ancs;
      boost::shared_ptr<const IComponent> current = m_base->getParent();
      while (current)
      {
        ancs.push_back( ParComponentFactory::create(current,m_map) );
        current = current->getParent();
      }
      return ancs;
    }

    /*! Set the name of the ParametrizedComponent (currently does nothing)
    *  @param s :: name string
    */
    void ParametrizedComponent::setName(const std::string& s)
    {
      (void) s; //Avoid compiler warnings
      //  name=s;
    }

    /*! Get the name of the ParametrizedComponent
    *  @return this.name
    */
    std::string ParametrizedComponent::getName() const
    {
      return m_base->getName();
    }

    /*! Set the position of the ParametrizedComponent
    *  The position is with respect to the parent ParametrizedComponent
    *  @param x :: x position
    *  @param y :: y position
    * 	@param z :: z position
    */
    void ParametrizedComponent::setPos(double x, double y, double z)
    {
      //  pos(x,y,z);
      (void) x; //Avoid compiler warning
      (void) y; //Avoid compiler warning
      (void) z; //Avoid compiler warning
    }

    /*! Set the position of the ParametrizedComponent
    *  The position is with respect to the parent ParametrizedComponent
    *  @param v :: vector position
    */
    void ParametrizedComponent::setPos(const V3D& v)
    {
      (void) v; //Avoid compiler warning
      //  pos=v;
    }

    /*! Set the orientation of the ParametrizedComponent
    *  The position is with respect to the parent ParametrizedComponent
    *  @param q :: rotation quaternion
    */
    void ParametrizedComponent::setRot(const Quat& q)
    {
      (void) q; //Avoid compiler warning
      //  rot=q;
    }

    /*! Copy the orientationmatrix from another ParametrizedComponent
    *  @param comp :: ParametrizedComponent to copy rotation from
    */
    void ParametrizedComponent::copyRot(const IComponent& comp)
    {
      (void) comp; //Avoid compiler warning
      //  rot=comp.rot;
    }

    /*! Translate the ParametrizedComponent relative to the parent ParametrizedComponent
    *  @param x :: translation on the x-axis
    * 	@param y :: translation on the y-axis
    *  @param z :: translation on the z-axis
    */
    void ParametrizedComponent::translate(double x, double y, double z)

    {
      //  pos[0]+=x;
      //  pos[1]+=y;
      //  pos[2]+=z;
      (void) x; //Avoid compiler warning
      (void) y; //Avoid compiler warning
      (void) z; //Avoid compiler warning
      return;
    }

    /*! Translate the ParametrizedComponent relative to the parent ParametrizedComponent
    *  @param v :: translation vector
    */
    void ParametrizedComponent::translate(const V3D& v)
    {
      //  pos+=v;
      (void) v; //Avoid compiler warning
      return;
    }

    /*! Rotate the ParametrizedComponent relative to the parent ParametrizedComponent
    *  @param r :: translation vector
    */
    void ParametrizedComponent::rotate(const Quat& r)
    {
      //  rot=r*rot;
      (void) r; //Avoid compiler warning
    }

    /*! Rotate the ParametrizedComponent by an angle in degrees with respect to an axis.
    * @param angle the number of degrees to rotate
    * @param axis The Vector to rotate around
    */
    void ParametrizedComponent::rotate(double angle, const V3D& axis)
    {
      //  throw Kernel::Exception::NotImplementedError("Rotate(double angle, const V3D& axis) has not been implemented");
      (void) angle; //Avoid compiler warning
      (void) axis; //Avoid compiler warning
    }

    /** Gets the position relative to the parent
    * @returns A vector of the relative position
    */
    V3D ParametrizedComponent::getRelativePos() const
    {
      Parameter_sptr par = m_map.get(m_base,"pos");
      if (par)
      {
        return par->value<V3D>();
      }
      return m_base->getRelativePos();
    }

    /** Get ScaleFactor of detector
    * @returns A vector of the scale factors (1,1,1) if not set
    */
    V3D ParametrizedComponent::getScaleFactorP() const
    {
      Parameter_sptr par = m_map.get(m_base,"sca");
      if (par)
      {
        return par->value<V3D>();
      }
      return V3D(1,1,1);
    }

    /** Gets the absolute position of the ParametrizedComponent
    * @returns A vector of the absolute position
    */
    V3D ParametrizedComponent::getPos() const
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

    /** Gets the rotation relative to the parent
    * @returns A quaternion of the relative rotation
    */
    const Quat& ParametrizedComponent::getRelativeRot() const
    {
      Parameter_sptr par = m_map.get(m_base,"rot");
      if (par)
      {
        return par->value<Quat>();
      }
      return m_base->getRelativeRot();
    }

    /** Returns the absolute rotation of the ParametrizedComponent
    *  @return A quaternion representing the total rotation
    */
    const Quat ParametrizedComponent::getRotation() const
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

    /** Gets the distance between two ParametrizedComponents
    *  @param comp The ParametrizedComponent to measure against
    *  @returns The distance
    */
    double ParametrizedComponent::getDistance(const IComponent& comp) const
    {
      return getPos().distance(comp.getPos());
    }

    /**
    * Get the names of the parameters for this component.
    * @param recursive If true, the parameters for all of the parent components are also included
    * @returns A set of strings giving the parameter names for this component
    */
    std::set<std::string> ParametrizedComponent::getParameterNames(bool recursive) const
    {
      std::set<std::string> names = m_map.names(this);
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
    bool ParametrizedComponent::hasParameter(const std::string & name, bool recursive) const
    {
      bool match_found(false);
      if( m_map.contains(this, name) )
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
    void ParametrizedComponent::printSelf(std::ostream& os) const
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
    * @param comp The ParametrizedComponent to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const ParametrizedComponent& comp)
    {
      comp.printSelf(os);
      return os;
    }

  } // Namespace Geometry

} // Namespace Mantid

