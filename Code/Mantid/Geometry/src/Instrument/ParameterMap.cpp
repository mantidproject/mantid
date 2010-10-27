#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
  namespace Geometry
  {

    // Get a reference to the logger
    Kernel::Logger& ParameterMap::g_log = Kernel::Logger::get("ParameterMap");

    //@cond
    /// Used in std::find_if as comparison opeartor.
    class equalParameterName
    {
    public:
      /// Constructor
      equalParameterName(const std::string& str):m_name(str){}
      /// Comparison operator
      bool operator()(const ParameterMap::pmap::value_type p)
      {
        return p.second->name() == m_name;
      }
    private:
      std::string m_name;///< parameter name
    };
    //@endcond

    /**
     * Copy constructor
     * @param copy A reference to the object whose contents should be copied.
     */
    ParameterMap::ParameterMap(const ParameterMap& copy)
      :m_map(copy.m_map)
    {
    }

    /**
     * Add a value into the map
     * @param type A string denoting the type, e.g. double, string, fitting
     * @param comp A pointer to the component that this parameter is attached to
     * @param name The name of the parameter
     * @param value The parameter's value
     */
    void ParameterMap::add(const std::string& type,const IComponent* comp,const std::string& name, const std::string& value)
    {
      boost::shared_ptr<Parameter> param = ParameterFactory::create(type,name);
      param->fromString(value);
      std::pair<pmap_it,pmap_it> range = m_map.equal_range(comp->getComponentID());
      for(pmap_it it=range.first;it!=range.second;++it)
      {
        if (it->second->name() == name)
        {
          it->second = param;
          return;
        }
      }
      m_map.insert(std::make_pair(comp->getComponentID(),param));
    }

    Parameter_sptr ParameterMap::get(const IComponent* comp,const std::string& name, const std::string & type) const
    {
      const ComponentID id = comp->getComponentID();
      pmap_cit it_found = m_map.find(id);
      if (it_found == m_map.end()) 
      {
        return Parameter_sptr();
      }

      pmap_cit itr = m_map.lower_bound(id);
      pmap_cit itr_end = m_map.upper_bound(id);

      const bool anytype = type.empty();
      for( ; itr != itr_end; ++itr )
      {
        Parameter_sptr param = itr->second;
        if( param->name() == name && (anytype || param->type() == type) )
        {
          return param;
        }
      }

      return Parameter_sptr();
    }

    Parameter_sptr ParameterMap::getRecursive(const IComponent* comp,const std::string& name, const std::string & type)const
    {
      boost::shared_ptr<const IComponent> compInFocus(comp,NoDeleting());
      while( compInFocus != NULL )
      {
        Parameter_sptr param = get(compInFocus.get(), name, type);
        if (param)
        {
          return param;
        }
        compInFocus = compInFocus->getParent();
      }

      return Parameter_sptr();
    }

    /**
     * Does the named parameter exist for the given component.
     * @param comp The component to be searched
     * @param name The name of the parameter
     * @param type An optional type. If provided, the parameter's type must also match
     * @returns A boolean indicating if the map contains the named parameter. If the type is given then
     * this must also match
     */
    bool ParameterMap::contains(const IComponent* comp, const std::string & name, const std::string & type) const
    {
      const ComponentID id = comp->getComponentID();
      std::pair<pmap_cit,pmap_cit> components = m_map.equal_range(id);
      bool anytype = type.empty();
      for( pmap_cit itr = components.first; itr != components.second; ++itr )
      {
        boost::shared_ptr<Parameter> param = itr->second;
        if( param->name() == name && (anytype || param->type() == type) )
        {
          return true;
        }
      }
      return false;
    }

    std::string ParameterMap::getString(const IComponent* comp,const std::string& name)
    {
      Parameter_sptr param = get(comp,name);
      if (!param) return "";
      return param->asString();
    }

    /** 
    * Returns a set with all the parameter names for the given component
    * @param comp A pointer to the component of interest
    * @returns A set of names of parameters for the given component
    */
    std::set<std::string> ParameterMap::names(const IComponent* comp)const
    {
      std::set<std::string> paramNames;
      const ComponentID id = comp->getComponentID();
      pmap_cit it_found = m_map.find(id);
      if (it_found == m_map.end()) 
      {
        return paramNames;
      }

      pmap_cit it_begin = m_map.lower_bound(id);
      pmap_cit it_end = m_map.upper_bound(id);

      for(pmap_cit it = it_begin; it != it_end; ++it)
      {
        paramNames.insert(it->second->name());
      }

      return paramNames;
    } 

    std::string ParameterMap::asString()const
    {
      std::stringstream out;
      for(pmap_cit it=m_map.begin();it!=m_map.end();it++)
      {
        boost::shared_ptr<Parameter> p = it->second;
        if (p && it->first)
        {
          const IComponent* comp = (const IComponent*)(it->first);
          const IDetector* det = dynamic_cast<const IDetector*>(comp);
          if (det)
          {
            out << "detID:"<<det->getID();
          }
          else
          {
            out << comp->getName();
          }
          out << ';' << p->type()<< ';' << p->name() << ';' << p->asString() << '|';
        }
      }
      return out.str();
    }

    /** Create or adjust "pos" parameter for a component
    * Assumed that name either equals "x", "y" or "z" otherwise this method will not add or modify "pos" parameter
    * @param comp Component
    * @param name name of the parameter
    * @param value value
    */
    void ParameterMap::addPositionCoordinate(const IComponent* comp, const std::string& name, const double value)
    {
      Parameter_sptr param = get(comp,"pos");
      V3D position;
      if (param)
      {
        // so "pos" already defined
        position = param->value<V3D>();
      }
      else
      {
        // so "pos" is not defined - therefore get position from component
        position = comp->getPos();
      }

      // adjust position

      if ( name.compare("x")==0 )
        position.setX(value);
      else if ( name.compare("y")==0 )
        position.setY(value);
      else if ( name.compare("z")==0 )
        position.setZ(value);
      else
      {
        g_log.warning() << "addPositionCoordinate() called with unrecognised coordinate symbol: " << name;
        return;
      }

      //clear the position cache
      clearCache();

      // finally add or update "pos" parameter

      if (param)
        param->set(position);
      else
        addV3D(comp, "pos", position);
    }

    /** Create or adjust "rot" parameter for a component
    * Assumed that name either equals "rotx", "roty" or "rotz" otherwise this method will not add/modify "rot" parameter
    @param comp Component
    @param name Parameter name
    @param deg Parameter value in degrees
    */
    void ParameterMap::addRotationParam(const IComponent* comp,const std::string& name, const double deg)
    {
      Parameter_sptr param = get(comp,"rot");
      Quat quat;

      Parameter_sptr paramRotX = get(comp,"rotx");
      Parameter_sptr paramRotY = get(comp,"roty");
      Parameter_sptr paramRotZ = get(comp,"rotz");
      double rotX, rotY, rotZ;

      if ( paramRotX )
        rotX = paramRotX->value<double>();
      else
        rotX = 0.0;

      if ( paramRotY )
        rotY = paramRotY->value<double>();
      else
        rotY = 0.0;

      if ( paramRotZ )
        rotZ = paramRotZ->value<double>();
      else
        rotZ = 0.0;
        

      // adjust rotation

      if ( name.compare("rotx")==0 )
      {
        if (paramRotX)
          paramRotX->set(deg);
        else
          addDouble(comp, "rotx", deg);

        quat = Quat(deg,V3D(1,0,0))*Quat(rotY,V3D(0,1,0))*Quat(rotZ,V3D(0,0,1));
      }
      else if ( name.compare("roty")==0 )
      {
        if (paramRotY)
          paramRotY->set(deg);
        else
          addDouble(comp, "roty", deg);

        quat = Quat(rotX,V3D(1,0,0))*Quat(deg,V3D(0,1,0))*Quat(rotZ,V3D(0,0,1));
      }
      else if ( name.compare("rotz")==0 )
      {
        if (paramRotZ)
          paramRotZ->set(deg);
        else
          addDouble(comp, "rotz", deg);

        quat = Quat(rotX,V3D(1,0,0))*Quat(rotY,V3D(0,1,0))*Quat(deg,V3D(0,0,1));
      }
      else
      {
        g_log.warning() << "addRotationParam() called with unrecognised coordinate symbol: " << name;
        return;
      }

      //clear the position cache
      clearCache();

      // finally add or update "pos" parameter
      if (param)
        param->set(quat);
      else
        addQuat(comp, "rot", quat);
    }


    /** @param str The error message
    */
    void ParameterMap::reportError(const std::string& str)
    {
      g_log.error(str);
    }

    ///Sets a cached location on the location cache
    /// @param comp The Component to set the location of
    /// @param location The location 
    void ParameterMap::setCachedLocation(const IComponent* comp, const V3D& location) const
    {
      // Call to setCachedLocation is a write so not thread-safe
      PARALLEL_CRITICAL(positionCache)
      {
        m_cacheLocMap.setCache(comp->getComponentID(),location);
      }
    }

    ///Attempts to retreive a location from the location cache
    /// @param comp The Component to find the location of
    /// @param location If the location is found it's value will be set here
    /// @returns true if the location is in the map, otherwise false
    bool ParameterMap::getCachedLocation(const IComponent* comp, V3D& location) const
    {
      return m_cacheLocMap.getCache(comp->getComponentID(),location);
    }

    ///Sets a cached rotation on the rotation cache
    /// @param comp The Component to set the rotation of
    /// @param rotation The rotation as a quaternion 
    void ParameterMap::setCachedRotation(const IComponent* comp, const Quat& rotation) const
    {
      // Call to setCachedRotation is a write so not thread-safe
      PARALLEL_CRITICAL(rotationCache)
      {
        m_cacheRotMap.setCache(comp->getComponentID(),rotation);
      }
    }

    ///Attempts to retreive a rotation from the rotation cache
    /// @param comp The Component to find the rotation of
    /// @param rotation If the rotation is found it's value will be set here
    /// @returns true if the rotation is in the map, otherwise false
    bool ParameterMap::getCachedRotation(const IComponent* comp, Quat& rotation) const
    {
      return m_cacheRotMap.getCache(comp->getComponentID(),rotation);
    }

    ///Sets a cached bounding box
    /// @param comp The Component to set the rotation of
    /// @param box A reference to the bounding box
    void ParameterMap::setCachedBoundingBox(const IComponent *comp, const BoundingBox & box) const
    {
      // Call to setCachedRotation is a write so not thread-safe
      PARALLEL_CRITICAL(boundingBoxCache)
      {
        m_boundingBoxMap.setCache(comp->getComponentID(), box);
      }
    }

    ///Attempts to retreive a bounding box from the cache
    /// @param comp The Component to find the bounding box of
    /// @param box If the bounding box is found it's value will be set here
    /// @returns true if the bounding is in the map, otherwise false
    bool ParameterMap::getCachedBoundingBox(const IComponent *comp, BoundingBox & box) const
    {
      return m_boundingBoxMap.getCache(comp->getComponentID(),box);
    }


  } // Namespace Geometry

} // Namespace Mantid

