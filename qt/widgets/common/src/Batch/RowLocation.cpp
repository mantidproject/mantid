#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <boost/algorithm/string/predicate.hpp>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

RowLocation::RowLocation(RowPath path) : m_path(std::move(path)) {}
RowPath const &RowLocation::path() const { return m_path; }
int RowLocation::rowRelativeToParent() const { return m_path.back(); }
bool RowLocation::isRoot() const { return m_path.empty(); }
bool RowLocation::isChildOf(RowLocation const &other) const {
  if (!isRoot()) {
    if (other.isRoot()) {
      return depth() == 1;
    } else {
      auto const &otherPath = other.path();
      if (depth() - other.depth() == 1)
        return std::equal(m_path.cbegin(), m_path.cend() - 1,
                          otherPath.cbegin(), otherPath.cend());
      else
        return false;
    }
  } else {
    return false;
  }
}

RowLocation RowLocation::parent() const {
  assertOrThrow(
      !isRoot(),
      "RowLocation::parent: cannot get parent of root node location.");
  return RowLocation(RowPath(m_path.begin(), m_path.cend() - 1));
}

RowLocation RowLocation::child(int n) const {
  auto childPath = RowPath(m_path);
  childPath.emplace_back(n);
  return RowLocation(childPath);
}

int RowLocation::depth() const { return static_cast<int>(m_path.size()); }

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

RowLocation RowLocation::relativeTo(RowLocation const &ancestor) const {
  assertOrThrow((*this).isDescendantOf(ancestor),
                "RowLocation::relativeTo: Tried to get position relative to "
                "node which was not an ancestor");
  return RowLocation(
      RowPath(m_path.cbegin() + ancestor.depth(), m_path.cend()));
}

using Row = std::vector<std::string>;
using Subtree = std::vector<std::pair<RowLocation, Row>>;

bool RowLocation::isSiblingOf(RowLocation const &other) const {
  if (!(isRoot() || other.isRoot())) {
    auto const &otherPath = other.path();
    if (depth() == other.depth())
      return std::equal(m_path.cbegin(), m_path.cend() - 1, otherPath.cbegin(),
                        otherPath.cend() - 1);
  }
  return false;
}

bool RowLocation::isChildOrSiblingOf(RowLocation const &other) const {
  return isChildOf(other) || isSiblingOf(other);
}

bool RowLocation::isDescendantOf(RowLocation const &ancestor) const {
  if (!isRoot()) {
    if (ancestor.isRoot()) {
      return true;
    } else {
      auto const &ancestorPath = ancestor.path();
      if (depth() > ancestor.depth())
        return std::equal(m_path.cbegin(), m_path.cbegin() + ancestor.depth(),
                          ancestorPath.cbegin(), ancestorPath.cend());
    }
  }
  return false;
}
}
}
}
