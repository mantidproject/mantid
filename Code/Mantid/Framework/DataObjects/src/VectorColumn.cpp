#include "MantidAPI/ColumnFactory.h"
#include "MantidDataObjects/VectorColumn.h"

namespace Mantid
{
namespace DataObjects
{
  // Please feel free to declare new types as you need them. Syntax is:
  // DECLARE_VECTORCOLUMN(type, name-of-the-type);
  
  // However, when you do that, please don't forget to add new type to the ITableWorkspace.cpp
  // so that it can be converted to Python list
  
  DECLARE_VECTORCOLUMN(int, vector_int)
  DECLARE_VECTORCOLUMN(double, vector_double)
} // namespace DataObjects
} // namespace Mantid
