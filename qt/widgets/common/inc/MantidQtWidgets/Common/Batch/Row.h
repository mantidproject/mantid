#ifndef MANTIDQTMANTIDWIDGETS_ROW_H_
#define MANTIDQTMANTIDWIDGETS_ROW_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include <boost/optional.hpp>
#include <ostream>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {


class EXPORT_OPT_MANTIDQT_COMMON Row {
public:
  Row(RowLocation location, std::vector<Cell> cells);

  std::vector<Cell> const &cells() const;
  std::vector<Cell> &cells();

  RowLocation const &location() const;

private:
  RowLocation m_location;
  std::vector<Cell> m_cells;
};

using Subtree = std::vector<Row>;

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os,
                                                    Row const &location);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<=(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>=(Row const &lhs, Row const &rhs);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
