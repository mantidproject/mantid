#ifndef MANTIDQTMANTIDWIDGETS_ROWLOCATION_H_
#define MANTIDQTMANTIDWIDGETS_ROWLOCATION_H_
#include <vector>
#include <ostream>
#include "MantidQtWidgets/Common/DllOption.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using RowPath = std::vector<int>;

class EXPORT_OPT_MANTIDQT_COMMON RowLocation {
public:
  RowLocation(RowPath path);
  RowPath const &path() const;
  int rowRelativeToParent() const;
  bool isRoot() const;

private:
  RowPath m_path;
};

std::ostream& EXPORT_OPT_MANTIDQT_COMMON operator<<(std::ostream& os, RowLocation const& location);
bool EXPORT_OPT_MANTIDQT_COMMON operator==(RowLocation const& lhs, RowLocation const& rhs);
bool EXPORT_OPT_MANTIDQT_COMMON operator!=(RowLocation const& lhs, RowLocation const& rhs);
bool EXPORT_OPT_MANTIDQT_COMMON operator<(RowLocation const& lhs, RowLocation const& rhs);
bool EXPORT_OPT_MANTIDQT_COMMON operator<=(RowLocation const& lhs, RowLocation const& rhs);
bool EXPORT_OPT_MANTIDQT_COMMON operator>(RowLocation const& lhs, RowLocation const& rhs);
bool EXPORT_OPT_MANTIDQT_COMMON operator>=(RowLocation const& lhs, RowLocation const& rhs);
}
}
}
#endif
