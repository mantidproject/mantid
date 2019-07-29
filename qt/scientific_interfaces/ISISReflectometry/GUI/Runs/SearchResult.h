// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_SEARCHRESULT_H
#define MANTID_ISISREFLECTOMETRY_SEARCHRESULT_H
#include "Common/DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL SearchResult {
public:
  SearchResult(const std::string &runNumber, const std::string &desc,
               const std::string &loc);

  std::string runNumber() const;
  std::string description() const;
  std::string location() const;
  std::string error() const;
  std::string groupName() const;
  std::string theta() const;
  void setError(std::string const &error);

private:
  std::string m_runNumber;
  std::string m_description;
  std::string m_location;
  std::string m_error;
  std::string m_groupName;
  std::string m_theta;

  void parseMetadata();
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(SearchResult const &lhs,
                                               SearchResult const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(SearchResult const &lhs,
                                               SearchResult const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_SEARCHRESULT_H
