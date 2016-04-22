#include "MantidDataObjects/TableColumn.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataObjects {

DECLARE_TABLECOLUMN(int, int)
DECLARE_TABLECOLUMN(uint32_t, uint)
DECLARE_TABLECOLUMN(int64_t, long64)
DECLARE_TABLECOLUMN(size_t, size_t)
DECLARE_TABLECOLUMN(float, float)
DECLARE_TABLECOLUMN(double, double)
DECLARE_TABLECOLUMN(API::Boolean, bool)
DECLARE_TABLECOLUMN(std::string, str)
DECLARE_TABLECOLUMN(Mantid::Kernel::V3D, V3D)

template <>
void TableColumn<std::string>::read(size_t index, const std::string &text) {
  /* As opposed to other types, assigning strings via a stream does not work if
   * it contains a whitespace character, so instead the assignment operator is
   * used.
   */
  m_data[index] = text;
}

} // namespace DataObjects
} // namespace Mantid
