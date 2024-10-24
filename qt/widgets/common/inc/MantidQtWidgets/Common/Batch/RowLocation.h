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
  bool operator==(RowLocation const &other) const;
  bool operator!=(RowLocation const &other) const;
  bool operator<(RowLocation const &other) const;
  bool operator<=(RowLocation const &other) const;
  bool operator>(RowLocation const &other) const;
  bool operator>=(RowLocation const &other) const;

private:
  RowPath m_path;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os, RowLocation const &location);
EXPORT_OPT_MANTIDQT_COMMON bool pathsSameUntilDepth(int depth, RowLocation const &locationA,
                                                    RowLocation const &locationB);

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
