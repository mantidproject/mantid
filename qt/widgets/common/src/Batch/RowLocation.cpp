// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <boost/algorithm/string/predicate.hpp>

#include "MantidQtWidgets/Common/Batch/equal.hpp"
// equivalent to
//         #include <boost/algorithm/cxx14/equal.hpp>
// or just #include <algorithm> in c++14
// available in boost 1.54+ - required for RHEL7.

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

RowLocation::RowLocation(RowPath path) : m_path(path) {}
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
        return boost::algorithm::equal(m_path.cbegin(), m_path.cend() - 1,
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

bool RowLocation::operator==(RowLocation const &other) const {
  return this->path() == other.path();
}

bool RowLocation::operator!=(RowLocation const &other) const {
  return !((*this) == other);
}

bool RowLocation::operator<(RowLocation const &other) const {
  auto &lhsPath = this->path();
  auto &rhsPath = other.path();
  return boost::algorithm::lexicographical_compare(lhsPath, rhsPath);
}

bool RowLocation::operator<=(RowLocation const &other) const {
  return (*this) < other || (*this) == other;
}

bool RowLocation::operator>=(RowLocation const &other) const {
  return !((*this) < other);
}

bool RowLocation::operator>(RowLocation const &other) const {
  return !((*this) <= other);
}

RowLocation RowLocation::relativeTo(RowLocation const &ancestor) const {
  assertOrThrow((*this) == ancestor || (*this).isDescendantOf(ancestor),
                "RowLocation::relativeTo: Tried to get position relative to "
                "node which was not an ancestor");
  return RowLocation(
      RowPath(m_path.cbegin() + ancestor.depth(), m_path.cend()));
}

bool RowLocation::isSiblingOf(RowLocation const &other) const {
  if (!(isRoot() || other.isRoot())) {
    auto const &otherPath = other.path();
    if (depth() == other.depth())
      return boost::algorithm::equal(m_path.cbegin(), m_path.cend() - 1,
                                     otherPath.cbegin(), otherPath.cend() - 1);
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
        return boost::algorithm::equal(
            m_path.cbegin(), m_path.cbegin() + ancestor.depth(),
            ancestorPath.cbegin(), ancestorPath.cend());
    }
  }
  return false;
}

bool pathsSameUntilDepth(int depth, RowLocation const &locationA,
                         RowLocation const &locationB) {
  auto &pathA = locationA.path();
  auto &pathB = locationB.path();
  assertOrThrow(depth <= std::min(locationA.depth(), locationB.depth()),
                "pathsSameUntilDepth: Comparison depth must be less than or "
                "equal to the depth of both locations");
  return boost::algorithm::equal(pathA.cbegin(), pathA.cbegin() + depth,
                                 pathB.cbegin(), pathB.cbegin() + depth);
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
