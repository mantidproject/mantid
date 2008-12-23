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

Parameter* ParameterMap::get(const IComponent* compName,const std::string& name)const
{
    pmap_it it_found = m_map.find(compName);
    if (it_found == m_map.end()) 
    {
        //g_log.error("Component "+compName+" does not have any parameters.");
        return 0;// ?
    }

    pmap_it it_begin = m_map.lower_bound(compName);
    pmap_it it_end = m_map.upper_bound(compName);

    pmap_it param = std::find_if(it_begin,it_end,equalParameterName(name));

    if ( param == it_end )
    {
        //g_log.error("Component "+compName+" does not have parameter "+name+".");
        return 0;// ?
    }
        
    return param->second.get();
}

std::string ParameterMap::getString(const IComponent* compName,const std::string& name)
{
    Parameter *param = get(compName,name);
    if (!param) return "";
    return param->asString();
}

std::vector<std::string> ParameterMap::nameList(const IComponent* compName)const
{
    std::vector<std::string> lst;

    pmap_it it_found = m_map.find(compName);
    if (it_found == m_map.end()) 
    {
        return lst;
    }

    pmap_it it_begin = m_map.lower_bound(compName);
    pmap_it it_end = m_map.upper_bound(compName);

    for(pmap_it it = it_begin;it!=it_end;it++)
        lst.push_back(it->second->name());

    return lst;

}

} // Namespace Geometry

} // Namespace Mantid

