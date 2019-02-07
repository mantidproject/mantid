// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_SEARCHRESULT_H
#define MANTID_ISISREFLECTOMETRY_SEARCHRESULT_H
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

struct SearchResult {
  SearchResult() {}
  SearchResult(const std::string &runNumber, const std::string &desc,
               const std::string &loc)
      : runNumber(runNumber), description(desc), location(loc) {}
  std::string runNumber;
  std::string description;
  std::string location;
  std::string issues;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_SEARCHRESULT_H
