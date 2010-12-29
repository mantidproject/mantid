#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include <sstream>

namespace Mantid
{
namespace Geometry
{

  /// @cond
  template<>
  void ParameterV3D::fromString(const std::string &value)
  {
    std::istringstream istr(value);
    m_value.readPrinted(istr);
  }
  /// @endcond

  ParameterFactory::FactoryMap ParameterFactory::s_map;

  /**  Creates an instance of a parameter
   *   @param className The parameter registered type name
   *   @param name The parameter name
   *   @return A pointer to the created parameter
   *   @throw runtime_error if the type has not been registered
   */
  boost::shared_ptr<Parameter> ParameterFactory::create(const std::string& className, const std::string& name)
  {
    boost::shared_ptr<Parameter> p;
    FactoryMap::const_iterator it = s_map.find(className);
    if (it != s_map.end())
      p = it->second->createInstance();
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
DECLARE_PARAMETER(string,std::string)
DECLARE_PARAMETER(V3D,Mantid::Geometry::V3D)
DECLARE_PARAMETER(Quat,Mantid::Geometry::Quat)
DECLARE_PARAMETER(fitting,Mantid::Geometry::FitParameter)
