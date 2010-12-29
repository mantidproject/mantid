#include "MantidDataObjects/TableColumn.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidGeometry/V3D.h"

namespace Mantid
{
namespace DataObjects
{

DECLARE_TABLECOLUMN(int,int)
DECLARE_TABLECOLUMN(float,float)
DECLARE_TABLECOLUMN(double,double)
DECLARE_TABLECOLUMN(API::Boolean,bool)
DECLARE_TABLECOLUMN(std::string,str)
DECLARE_TABLECOLUMN(Mantid::Geometry::V3D,V3D)
DECLARE_TABLECOLUMN(int64_t,long64)


} // namespace DataObjects
} // namespace Mantid

