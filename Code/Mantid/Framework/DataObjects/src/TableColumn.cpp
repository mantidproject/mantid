#include "MantidDataObjects/TableColumn.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidKernel/V3D.h"

namespace Mantid
{
namespace DataObjects
{

DECLARE_TABLECOLUMN(int,int)
DECLARE_TABLECOLUMN(uint32_t,uint)
DECLARE_TABLECOLUMN(int64_t,long64)
DECLARE_TABLECOLUMN(size_t,size_t)
DECLARE_TABLECOLUMN(float,float)
DECLARE_TABLECOLUMN(double,double)
DECLARE_TABLECOLUMN(API::Boolean,bool)
DECLARE_TABLECOLUMN(std::string,str)
DECLARE_TABLECOLUMN(Mantid::Kernel::V3D,V3D)

} // namespace DataObjects
} // namespace Mantid

