#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include <boost/algorithm/string/predicate.hpp>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

RowLocation::RowLocation(RowPath path) : m_path(std::move(path)) {}
RowPath const &RowLocation::path() const { return m_path; }
int RowLocation::rowRelativeToParent() const { return m_path.back(); }
bool RowLocation::isRoot() const { return m_path.empty(); }
std::size_t RowLocation::depth() const { return m_path.size(); }

std::ostream &operator<<(std::ostream &os, RowLocation const &location) {
  auto &path = location.path();
  os << "[";
  if (!path.empty()) {
    auto it = path.cbegin();
    for (; it < path.cend() - 1; ++it)
      os << (*it) << ", ";
    os << (*it);
  }
  os << "]";
  return os;
}

bool operator==(RowLocation const &lhs, RowLocation const &rhs) {
  return lhs.path() == rhs.path();
}

bool operator!=(RowLocation const &lhs, RowLocation const &rhs) {
  return !(lhs == rhs);
}

bool operator<(RowLocation const &lhs, RowLocation const &rhs) {
  auto &lhsPath = lhs.path();
  auto &rhsPath = rhs.path();
  return boost::algorithm::lexicographical_compare(lhsPath, rhsPath);
}

bool operator<=(RowLocation const &lhs, RowLocation const &rhs) {
  return lhs < rhs || lhs == rhs;
}

bool operator>=(RowLocation const &lhs, RowLocation const &rhs) {
  return !(lhs < rhs);
}

bool operator>(RowLocation const &lhs, RowLocation const &rhs) {
  return !(lhs <= rhs);
}
}
}
}
