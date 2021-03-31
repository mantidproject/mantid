// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RowLocation.h"
#include "Common/Map.h"
#include <algorithm>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename T> void sortAndRemoveDuplicatesInplace(std::vector<T> &items) {
  std::sort(items.begin(), items.end());
  auto eraseBegin = std::unique(items.begin(), items.end());
  items.erase(eraseBegin, items.end());
}

std::vector<int> groupIndexesFromSelection(std::vector<MantidWidgets::Batch::RowLocation> const &selected) {
  auto groups = mapToContainingGroups(selected);
  sortAndRemoveDuplicatesInplace(groups);
  return groups;
}

std::vector<int>
mapToContainingGroups(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &mustNotContainRoot) {
  return map(mustNotContainRoot,
             [](MantidWidgets::Batch::RowLocation const &location) -> int { return groupOf(location); });
}

bool containsGroups(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations) {
  return std::any_of(
      locations.cbegin(), locations.cend(),
      [](MantidQt::MantidWidgets::Batch::RowLocation const &location) -> bool { return isGroupLocation(location); });
}

bool isGroupLocation(MantidQt::MantidWidgets::Batch::RowLocation const &location) { return location.depth() == 1; }

int groupOf(MantidQt::MantidWidgets::Batch::RowLocation const &groupLocation) { return groupLocation.path()[0]; }

bool isRowLocation(MantidWidgets::Batch::RowLocation const &location) { return location.depth() == 2; }

int rowOf(MantidQt::MantidWidgets::Batch::RowLocation const &rowLocation) { return rowLocation.path()[1]; }

bool containsPath(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations,
                  MantidQt::MantidWidgets::Batch::RowPath const &path) {
  return std::any_of(
      locations.cbegin(), locations.cend(),
      [&path](MantidQt::MantidWidgets::Batch::RowLocation const &location) -> bool { return location.path() == path; });
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
