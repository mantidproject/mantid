#include "MantidDataObjects/TableColumn.h"
#include "MAntidDataObjects/ColumnFactory.h"
#include "MantidGeometry/V3D.h"

namespace Mantid
{
namespace DataObjects
{

DECLARE_TABLECOLUMN(int,int)
DECLARE_TABLECOLUMN(float,float)
DECLARE_TABLECOLUMN(double,double)
DECLARE_TABLECOLUMN(bool,bool)
DECLARE_TABLECOLUMN(std::string,str)
DECLARE_TABLECOLUMN(Mantid::Geometry::V3D,V3D)

} // namespace DataObjects
} // namespace Mantid


///\endcond TEMPLATE
