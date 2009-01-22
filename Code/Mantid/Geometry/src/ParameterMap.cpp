#include "MantidGeometry/ParameterMap.h"

namespace Mantid
{
namespace Geometry
{

// Get a reference to the logger
Kernel::Logger& ParameterMap::g_log = Kernel::Logger::get("ParameterMap");

class equalParameterName
{
public:
    equalParameterName(const std::string& str):m_name(str){}
    bool operator()(const ParameterMap::pmap::value_type p)
    {
        return p.second->name() == m_name;
    }
private:
    std::string m_name;
};

Parameter_sptr ParameterMap::get(const IComponent* comp,const std::string& name)const
{
    pmap_it it_found = m_map.find(comp);
    if (it_found == m_map.end()) 
    {
        //g_log.error("Component "+comp+" does not have any parameters.");
        return Parameter_sptr();// ?
    }

    pmap_it it_begin = m_map.lower_bound(comp);
    pmap_it it_end = m_map.upper_bound(comp);

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

    pmap_it it_found = m_map.find(comp);
    if (it_found == m_map.end()) 
    {
        return lst;
    }

    pmap_it it_begin = m_map.lower_bound(comp);
    pmap_it it_end = m_map.upper_bound(comp);

    for(pmap_it it = it_begin;it!=it_end;it++)
        lst.push_back(it->second->name());

    return lst;

}

} // Namespace Geometry

} // Namespace Mantid

