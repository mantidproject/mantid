#include "MantidDataObjects/TableColumn.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidKernel/V3D.h"

namespace Mantid
{
namespace DataObjects
{

template<>
double TableColumn<API::Boolean>::toDouble(size_t i)const
{
  return m_data[i] ? 1.0 : 0.0;
}

template<>
void TableColumn<API::Boolean>::fromDouble(size_t i, double value)
{
  m_data[i] = value != 0.0;
}

DECLARE_TABLECOLUMN(int,int)
DECLARE_TABLECOLUMN(float,float)
DECLARE_TABLECOLUMN(double,double)
DECLARE_TABLECOLUMN(API::Boolean,bool)
DECLARE_TABLECOLUMN(std::string,str)
DECLARE_TABLECOLUMN(Mantid::Kernel::V3D,V3D)
DECLARE_TABLECOLUMN(int64_t,long64)


} // namespace DataObjects
} // namespace Mantid

