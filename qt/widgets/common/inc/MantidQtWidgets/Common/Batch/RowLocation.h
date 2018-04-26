#ifndef MANTIDQTMANTIDWIDGETS_ROWLOCATION_H_
#define MANTIDQTMANTIDWIDGETS_ROWLOCATION_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <ostream>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using RowPath = std::vector<int>;

class EXPORT_OPT_MANTIDQT_COMMON RowLocation {
public:
  RowLocation() = default;
  RowLocation(RowPath path);
  RowPath const &path() const;
  int rowRelativeToParent() const;
  bool isRoot() const;
  int depth() const;
  bool isChildOf(RowLocation const &other) const;
  bool isSiblingOf(RowLocation const &other) const;
  bool isChildOrSiblingOf(RowLocation const &other) const;
  bool isDescendantOf(RowLocation const &other) const;
  RowLocation parent() const;
  RowLocation relativeTo(RowLocation const &ancestor) const;
  RowLocation child(int n) const;

private:
  RowPath m_path;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &
operator<<(std::ostream &os, RowLocation const &location);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(RowLocation const &lhs,
                                           RowLocation const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(RowLocation const &lhs,
                                           RowLocation const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<(RowLocation const &lhs,
                                          RowLocation const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<=(RowLocation const &lhs,
                                           RowLocation const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>(RowLocation const &lhs,
                                          RowLocation const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>=(RowLocation const &lhs,
                                           RowLocation const &rhs);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
