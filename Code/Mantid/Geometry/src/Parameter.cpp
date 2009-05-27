#include "MantidGeometry/Parameter.h"
#include "MantidGeometry/ParameterFactory.h"
#include <sstream>

namespace Mantid
{
namespace Geometry
{

  template<>
  void ParameterV3D::fromString(const std::string &value)
  {
    std::istringstream istr(value);
    m_value.readPrinted(istr);
  }


    ParameterFactory::FactoryMap ParameterFactory::s_map;

    Parameter* ParameterFactory::create(const std::string& className, const std::string& name)
    {
        Parameter* p = NULL;
        FactoryMap::const_iterator it = s_map.find(className);
        if (it != s_map.end())
            p = it->second->createUnwrappedInstance();
        else
            throw std::runtime_error("ParameterFactory:"+ className + " is not registered.\n");
        p->m_name = name;
        p->m_type = className;
        return p;
    }

} // Namespace Geometry

} // Namespace Mantid

DECLARE_PARAMETER(int,int)
DECLARE_PARAMETER(double,double)
DECLARE_PARAMETER(bool,bool)
DECLARE_PARAMETER(str,std::string)
DECLARE_PARAMETER(V3D,Mantid::Geometry::V3D)
DECLARE_PARAMETER(Quat,Mantid::Geometry::Quat)

