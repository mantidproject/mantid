#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/ColumnFactory.h"
#include "MantidGeometry/V3D.h"

namespace Mantid
{
namespace DataObjects
{

DECLARE_TABLECOLUMN(int,int)
DECLARE_TABLECOLUMN(float,float)
DECLARE_TABLECOLUMN(double,double)
DECLARE_TABLECOLUMN(Boolean,bool)
DECLARE_TABLECOLUMN(std::string,str)
DECLARE_TABLECOLUMN(Mantid::Geometry::V3D,V3D)

DLLExport std::ostream& operator<<(std::ostream& s,const Boolean& b)
{
    s << (b.value?"true":"false");
    return s;
}

} // namespace DataObjects
} // namespace Mantid

