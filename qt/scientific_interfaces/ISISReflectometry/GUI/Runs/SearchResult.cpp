// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SearchResult.h"

namespace MantidQt {
namespace CustomInterfaces {
bool operator==(SearchResult const &lhs, SearchResult const &rhs) {
  // Ignore the issues field in the comparison because this represents the
  // state of the item and is not a unique identifier
  return lhs.runNumber == rhs.runNumber && lhs.description == rhs.description &&
         lhs.location == rhs.location;
}

bool operator!=(SearchResult const &lhs, SearchResult const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
