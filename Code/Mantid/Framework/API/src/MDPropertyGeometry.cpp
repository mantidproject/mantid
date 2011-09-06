#include "MantidAPI/MDPropertyGeometry.h"
//#include "MantidAPI/

namespace Mantid
{
namespace API
{
std::string 
MDPropertyGeometry::setValue(const Geometry::MDGeometryOld &origin)
{
  this->build_from_geometry(origin);
  return this->Kernel::PropertyWithValue<std::string>::setValue(this->toXMLstring());
}
std::string 
MDPropertyGeometry::setValue(const std::string &XMLstring)
{
  if(this->fromXMLstring(XMLstring)){
    return this->Kernel::PropertyWithValue<std::string>::setValue(XMLstring);
  }
  return "";
}
//static MDPropertyGeometry ws("test_ws","aa",Kernel::Direction::Input);
//@cond TEMPLATE
//template MANTID_API_DLL class Mantid::API::WorkspaceProperty<Mantid::API::Workspace>;
//@endcond TEMPLATE
} // namespace API
} // 

/*
namespace Mantid
{
namespace Kernel
{

/// @cond

template MANTID_API_DLL class PropertyWithValue<Geometry::MDGeometryDescription>;


/// @endcond

} // namespace Kernel
} // namespace Mant
*/
