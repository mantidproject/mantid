// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/DllOption.h"
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

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os, Row const &location);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<=(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>=(Row const &lhs, Row const &rhs);

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
