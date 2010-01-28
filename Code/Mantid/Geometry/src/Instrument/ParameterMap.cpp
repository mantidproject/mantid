#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
namespace Geometry
{

// Get a reference to the logger
Kernel::Logger& ParameterMap::g_log = Kernel::Logger::get("ParameterMap");

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

ParameterMap::ParameterMap(const ParameterMap& copy)
:m_map(copy.m_map)
{
}

Parameter_sptr ParameterMap::get(const IComponent* comp,const std::string& name)const
{
    const ComponentID id = comp->getComponentID();
    pmap_it it_found = m_map.find(id);
    if (it_found == m_map.end()) 
    {
        //g_log.error("Component "+comp+" does not have any parameters.");
        return Parameter_sptr();// ?
    }

    pmap_it it_begin = m_map.lower_bound(id);
    pmap_it it_end = m_map.upper_bound(id);

    pmap_it param = std::find_if(it_begin,it_end,equalParameterName(name));

    if ( param == it_end )
    {
        //g_log.error("Component "+comp+" does not have parameter "+name+".");
        return Parameter_sptr();// ?
    }
        
    return param->second;
}

std::string ParameterMap::getString(const IComponent* comp,const std::string& name)
{
    Parameter_sptr param = get(comp,name);
    if (!param) return "";
    return param->asString();
}

std::vector<std::string> ParameterMap::nameList(const IComponent* comp)const
{
    std::vector<std::string> lst;

    const ComponentID id = comp->getComponentID();

    pmap_it it_found = m_map.find(id);
    if (it_found == m_map.end()) 
    {
        return lst;
    }

    pmap_it it_begin = m_map.lower_bound(id);
    pmap_it it_end = m_map.upper_bound(id);

    for(pmap_it it = it_begin;it!=it_end;it++)
        lst.push_back(it->second->name());

    return lst;

} 

std::string ParameterMap::asString()const
{
    std::stringstream out;
    for(pmap_it it=m_map.begin();it!=m_map.end();it++)
    {
        boost::shared_ptr<Parameter> p = it->second;
        if (p && it->first)
        {
            out << ((const IComponent*)(it->first))->getName() << ';' << p->type()<< ';' << p->name() << ';' << p->asString() << '|';
        }
    }
    return out.str();
}


/** Create or adjust "pos" parameter for a component
  * Assumed that name either equals "x", "y" or "z" otherwise this method will not add/modify "pos" parameter
    @param comp Component
    @param name Parameter name
    @param value Parameter value
  */
void ParameterMap::addPositionCoordinate(const IComponent* comp,const std::string& name, const double value)
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

/// Create or adjust "rot" parameter for a component
void ParameterMap::addRotationParam(const IComponent* comp,const std::string& name, const double deg)
{
  Parameter_sptr param = get(comp,"rot");
  Quat quat;
  if (param)
  {
    // so "rot" already defined
    quat = param->value<Quat>();
  }
  else
  {
    // so "rot" is not defined - therefore get quarternion from component
    quat = comp->getRelativeRot();
  }

  // adjust rotation

  if ( name.compare("rotx")==0 )
  {
      quat = Quat(deg,V3D(1,0,0))*quat;
  }
  else if ( name.compare("roty")==0 )
  {
      quat = Quat(deg,V3D(0,1,0))*quat;
  }
  else if ( name.compare("rotz")==0 )
  {
      quat = Quat(deg,V3D(0,0,1))*quat;
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
void ParameterMap::setCachedLocation(const IComponent* comp, V3D& location) const
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
  bool result;
  // Call to getCachedLocation is a needs to be marked as the same critical region as a 
  // write to the cache
  PARALLEL_CRITICAL(positionCache)
  {
    result = m_cacheLocMap.getCache(comp->getComponentID(),location);
  }
  return result;
}

///Sets a cached rotation on the rotation cache
/// @param comp The Component to set the rotation of
/// @param rotation The rotation as a quaternion 
void ParameterMap::setCachedRotation(const IComponent* comp, Quat& rotation) const
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
  bool result;
  // Call to getCachedRotation is a needs to be marked as the same critical region as a 
  // write to the cache
  PARALLEL_CRITICAL(rotationCache)
  {
    result = m_cacheRotMap.getCache(comp->getComponentID(),rotation);
  }
  return result;
}


} // Namespace Geometry

} // Namespace Mantid

