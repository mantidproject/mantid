#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include <cstring>

namespace Mantid
{
  namespace Geometry
  {
    // Get a reference to the logger
    Kernel::Logger& ParameterMap::g_log = Kernel::Logger::get("ParameterMap");

    //--------------------------------------------------------------------------
    // Public method
    //--------------------------------------------------------------------------
    /**
     * Default constructor
     */
    ParameterMap::ParameterMap()
      : m_spectraMap(NULL), m_map(), m_nearestNeighbours()
    {}

    /**
     * Constructor
     * @param spectraMap :: A pointer to the spectra map
     */
    ParameterMap::ParameterMap(const ISpectraDetectorMap *const spectraMap) 
      : m_spectraMap(spectraMap), m_map(), m_nearestNeighbours()
    {}

    /**
     * Add a value into the map
     * @param type :: A string denoting the type, e.g. double, string, fitting
     * @param comp :: A pointer to the component that this parameter is attached to
     * @param name :: The name of the parameter
     * @param value :: The parameter's value
     */
    void ParameterMap::add(const std::string& type,const IComponent* comp,const std::string& name, 
                           const std::string& value)
    {
      PARALLEL_CRITICAL(parameter_add)
      {
        bool created(false);
        boost::shared_ptr<Parameter> param = retrieveParameter(created, type, comp, name);
        param->fromString(value);
        if( created )
        {
          m_map.insert(std::make_pair(comp->getComponentID(),param));
        }
      }
    }

    /** Create or adjust "pos" parameter for a component
     * Assumed that name either equals "x", "y" or "z" otherwise this 
     * method will not add or modify "pos" parameter
     * @param comp :: Component
     * @param name :: name of the parameter
     * @param value :: value
     */
    void ParameterMap::addPositionCoordinate(const IComponent* comp, const std::string& name, 
                                             const double value)
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
     * Assumed that name either equals "rotx", "roty" or "rotz" otherwise this 
     * method will not add/modify "rot" parameter
     * @param comp :: Component
     * @param name :: Parameter name
     * @param deg :: Parameter value in degrees
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

    /**  
     * Adds a double value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a string
    */
    void ParameterMap::addDouble(const IComponent* comp,const std::string& name, const std::string& value)
    {
      add("double",comp,name,value);
    }

    /**
     * Adds a double value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a double
    */
    void ParameterMap::addDouble(const IComponent* comp,const std::string& name, double value)
    {
      add("double",comp,name,value);
    }

    /**  
     * Adds an int value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a string
     */
    void ParameterMap::addInt(const IComponent* comp,const std::string& name, const std::string& value)
    {
      add("int",comp,name,value);
    }

    /**
     * Adds an int value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as an int
     */
    void ParameterMap::addInt(const IComponent* comp,const std::string& name, int value)
    {
      add("int",comp,name,value);
    }

    /**  
     * Adds a bool value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a string
     */
    void ParameterMap::addBool(const IComponent* comp,const std::string& name, const std::string& value)
    {
      add("bool",comp,name,value);
    }
    /**
     * Adds a bool value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a bool
     */
    void ParameterMap::addBool(const IComponent* comp,const std::string& name, bool value)
    {
      add("bool",comp,name,value);
    }

    /**  
     * Adds a std::string value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value
    */
    void ParameterMap::addString(const IComponent* comp,const std::string& name, const std::string& value)
    {
      add<std::string>("string",comp,name,value);
    }

    /**  
     * Adds a V3D value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a string
     */
    void ParameterMap::addV3D(const IComponent* comp,const std::string& name, const std::string& value)
    {
      add("V3D",comp,name,value);
      clearCache();
    }

    /**
     * Adds a V3D value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a V3D
    */
    void ParameterMap::addV3D(const IComponent* comp,const std::string& name, const V3D& value)
    {
      add("V3D",comp,name,value);
      clearCache();
    }

    /**  
     * Adds a Quat value to the parameter map.
     * @param comp :: Component to which the new parameter is related
     * @param name :: Name for the new parameter
     * @param value :: Parameter value as a Quat
    */
    void ParameterMap::addQuat(const IComponent* comp,const std::string& name, const Quat& value)
    {
      add("Quat",comp,name,value);
      clearCache();
    }

    /**
     * FASTER LOOKUPin multithreaded loops . Does the named parameter exist for the given component. 
     * In a multi-threaded loop this yields much better performance than the counterpart
     * using std::string as it does not dynamically allocate any memory
     * @param comp :: The component to be searched
     * @param name :: The name of the parameter
     * @returns A boolean indicating if the map contains the named parameter. If the type is given then
     * this must also match
     */
    bool ParameterMap::contains(const IComponent* comp, const char * name) const
    {
      if( m_map.empty() ) return false;
      const ComponentID id = comp->getComponentID();
      std::pair<pmap_cit,pmap_cit> components = m_map.equal_range(id);
      for( pmap_cit itr = components.first; itr != components.second; ++itr )
      {
        if( strcmp(itr->second->nameAsCString(), name) == 0 )
        {
          return true;
        }
      }
      return false;
    }

    /**
     * SLOWER VERSION in multithreaded loops. Does the named parameter exist for the given component
     * and given type
     * @param comp :: The component to be searched
     * @param name :: The name of the parameter
     * @param type :: The type of the component as a string
     * @returns A boolean indicating if the map contains the named parameter. If the type is given then
     * this must also match
     */
    bool ParameterMap::contains(const IComponent* comp, const std::string & name, 
                                const std::string & type) const
    {
      if( m_map.empty() ) return false;
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

    /** FASTER LOOKUP in multithreaded loops. Return a named parameter
     * @param comp :: Component to which parameter is related
     * @param name :: Parameter name
     * @returns The named parameter if it exists or a NULL shared pointer if not
     */
    Parameter_sptr ParameterMap::get(const IComponent* comp, const char* name) const
    {
      if( m_map.empty() ) return Parameter_sptr();
      const ComponentID id = comp->getComponentID();
      pmap_cit it_found = m_map.find(id);
      if (it_found == m_map.end())
      {
        return Parameter_sptr();
      }
      pmap_cit itr = m_map.lower_bound(id);
      pmap_cit itr_end = m_map.upper_bound(id);
      for( ; itr != itr_end; ++itr )
      {
        Parameter_sptr param = itr->second;
        if( strcmp(param->nameAsCString(), name) == 0 )
        {
          return param;
        }
      }
      return Parameter_sptr();
    }


    /** SLOWER LOOKUP in multithreaded loops. Return a named parameter of a given type
     * @param comp :: Component to which parameter is related
     * @param name :: Parameter name
     * @param type :: An optional type string
     * @returns The named parameter of the given type if it exists or a NULL shared pointer if not
     */
    Parameter_sptr ParameterMap::get(const IComponent* comp, const std::string& name, 
                                     const std::string & type) const
    {
      if( m_map.empty() ) return Parameter_sptr();
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

    /** FASTER LOOKUP in multithreaded loops. Find a parameter by name, recursively going up
     * the component tree to higher parents.
     * @param comp :: The component to start the search with
     * @param name :: Parameter name
     * @returns the first matching parameter.
     */
    Parameter_sptr ParameterMap::getRecursive(const IComponent* comp, const char * name) const
    {
      boost::shared_ptr<const IComponent> compInFocus(comp,NoDeleting());
      while( compInFocus != NULL )
      {
        Parameter_sptr param = get(compInFocus.get(), name);
        if (param)
        {
          return param;
        }
        compInFocus = compInFocus->getParent();
      }
      //Nothing was found!
      return Parameter_sptr();
    }

    /** 
     * Find a parameter by name, recursively going up the component tree
     * to higher parents.
     * @param comp :: The component to start the search with
     * @param name :: Parameter name
     * @param type :: An optional type string
     * @returns the first matching parameter.
     */
    Parameter_sptr ParameterMap::getRecursive(const IComponent* comp,const std::string& name, 
                                              const std::string & type) const
    {
      if( m_map.empty() ) return Parameter_sptr();
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
      //Nothing was found!
      return Parameter_sptr();
    }

    /**  
     * Return the value of a parameter as a string
     * @param comp :: Component to which parameter is related
     * @param name :: Parameter name
     * @return string representation of the parameter
     */
    std::string ParameterMap::getString(const IComponent* comp, const std::string& name)
    {
      Parameter_sptr param = get(comp,name);
      if (!param) return "";
      return param->asString();
    }

    /** 
     * Returns a set with all the parameter names for the given component
     * @param comp :: A pointer to the component of interest
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
    
    /**
     * Return a string representation of the parameter map. The format is either:
     * |detID:id-value;param-type;param-name;param-value| for a detector or
     * |comp-name;param-type;param-name;param-value| for other components.
     * @returns A string containing the contents of the parameter map.
     */
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

    /**
     * Clears the cache and nearest neighbour information managed by the parameter map.
     */
    void ParameterMap::clearCache()
    {
      m_cacheLocMap.clear();
      m_cacheRotMap.clear();
      m_boundingBoxMap.clear();
      m_nearestNeighbours.reset();
    }
 
    ///Sets a cached location on the location cache
    /// @param comp :: The Component to set the location of
    /// @param location :: The location 
    void ParameterMap::setCachedLocation(const IComponent* comp, const V3D& location) const
    {
      // Call to setCachedLocation is a write so not thread-safe
      PARALLEL_CRITICAL(positionCache)
      {
        m_cacheLocMap.setCache(comp->getComponentID(),location);
      }
    }

    ///Attempts to retreive a location from the location cache
    /// @param comp :: The Component to find the location of
    /// @param location :: If the location is found it's value will be set here
    /// @returns true if the location is in the map, otherwise false
    bool ParameterMap::getCachedLocation(const IComponent* comp, V3D& location) const
    {
      return m_cacheLocMap.getCache(comp->getComponentID(),location);
    }

    ///Sets a cached rotation on the rotation cache
    /// @param comp :: The Component to set the rotation of
    /// @param rotation :: The rotation as a quaternion 
    void ParameterMap::setCachedRotation(const IComponent* comp, const Quat& rotation) const
    {
      // Call to setCachedRotation is a write so not thread-safe
      PARALLEL_CRITICAL(rotationCache)
      {
        m_cacheRotMap.setCache(comp->getComponentID(),rotation);
      }
    }

    ///Attempts to retreive a rotation from the rotation cache
    /// @param comp :: The Component to find the rotation of
    /// @param rotation :: If the rotation is found it's value will be set here
    /// @returns true if the rotation is in the map, otherwise false
    bool ParameterMap::getCachedRotation(const IComponent* comp, Quat& rotation) const
    {
      return m_cacheRotMap.getCache(comp->getComponentID(),rotation);
    }

    ///Sets a cached bounding box
    /// @param comp :: The Component to set the rotation of
    /// @param box :: A reference to the bounding box
    void ParameterMap::setCachedBoundingBox(const IComponent *comp, const BoundingBox & box) const
    {
      // Call to setCachedRotation is a write so not thread-safe
      PARALLEL_CRITICAL(boundingBoxCache)
      {
        m_boundingBoxMap.setCache(comp->getComponentID(), box);
      }
    }

    ///Attempts to retreive a bounding box from the cache
    /// @param comp :: The Component to find the bounding box of
    /// @param box :: If the bounding box is found it's value will be set here
    /// @returns true if the bounding is in the map, otherwise false
    bool ParameterMap::getCachedBoundingBox(const IComponent *comp, BoundingBox & box) const
    {
      return m_boundingBoxMap.getCache(comp->getComponentID(),box);
    }

    //--------------------------------------------------------------------------
    // Nearest Neighbours code. This may be moved after a rethink of where the neigbours map
    // should live
    //--------------------------------------------------------------------------
    void ParameterMap::resetSpectraMap(const ISpectraDetectorMap *const spectramap)
    {
      m_spectraMap = spectramap;
      // The neighbour map needs to be rebuilt
      m_nearestNeighbours.reset();
    }

    /**
     * Handles the building of the NearestNeighbours object, if it has not already been 
     * populated for this parameter map.
     * @param comp :: Object used for determining the Instrument
     */
    void ParameterMap::buildNearestNeighbours(const IComponent *comp) const
    {
      if( !m_spectraMap )
      {
	throw Kernel::Exception::NullPointerException("ParameterMap::buildNearestNeighbours", 
						      "SpectraDetectorMap");
      }

      if ( !m_nearestNeighbours )
      {
        // Get pointer to Instrument
        boost::shared_ptr<const IComponent> parent(comp, NoDeleting());
        while ( parent->getParent() )
        {
          parent = parent->getParent();
        }
        boost::shared_ptr<const Instrument> inst = boost::dynamic_pointer_cast<const Instrument>(parent);
        if ( inst )
        {
          m_nearestNeighbours.reset(new NearestNeighbours(inst, *m_spectraMap));
        }
        else
        {
          throw Mantid::Kernel::Exception::NullPointerException("ParameterMap: buildNearestNeighbours", 
								parent->getName());
        }
      }
    }
    
    /**
     * Queries the NearestNeighbours object for the selected detector.
     * @param comp :: pointer to the querying detector
     * @param radius :: distance from detector on which to filter results
     * @return map of DetectorID to distance for the nearest neighbours
     */
    std::map<specid_t, double> ParameterMap::getNeighbours(const IDetector *comp, const double radius) const
    {
      if ( !m_nearestNeighbours )
      {
        buildNearestNeighbours(comp);
      }
      // Find the spectrum number
      std::vector<specid_t> spectra = m_spectraMap->getSpectra(std::vector<detid_t>(1, comp->getID()));
      if(spectra.empty())
      {
	throw Kernel::Exception::NotFoundError("ParameterMap::getNeighbours - Cannot find spectrum number for detector", comp->getID());
      }
      std::map<specid_t, double> neighbours = m_nearestNeighbours->neighbours(spectra[0], radius);
      return neighbours;      
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /**
     *  Retrieve a parameter by either creating a new one of getting an existing one
     * @param[out] created Set to true if the named parameter was newly created, false otherwise
     * @param type :: A string denoting the type, e.g. double, string, fitting
     * @param comp :: A pointer to the component that this parameter is attached to
     * @param name :: The name of the parameter
     */
    Parameter_sptr ParameterMap::retrieveParameter(bool & created, const std::string & type, 
                                                   const IComponent* comp, const std::string & name)
    {
      boost::shared_ptr<Parameter> param;
      if( this->contains(comp, name) )
      {
        param = this->get(comp, name);
        if( param->type() != type )
        {
          reportError("ParameterMap::add - Type mismatch on replacement of '" + name + "' parameter");
          throw std::runtime_error("ParameterMap::add - Type mismatch on parameter replacement");
        }
        created = false;
      }
      else
      {
        // Create a new one
        param = ParameterFactory::create(type,name);
        created = true;
      }
      return param;
    }
    
    /** Logs an error
     *  @param str :: The error message
     */
    void ParameterMap::reportError(const std::string& str)
    {
      g_log.error(str);
    }

  } // Namespace Geometry
} // Namespace Mantid

