// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SearchResult.h"
#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {

SearchResult::SearchResult(const std::string &runNumber,
                           const std::string &desc, const std::string &loc)
    : m_runNumber(runNumber), m_description(desc), m_location(loc),
      m_groupName(), m_theta() {
  parseMetadata();
}

void SearchResult::parseMetadata() {
  static boost::regex descriptionFormatRegex("(.*)(th[:=]([0-9.]+))(.*)");
  boost::smatch matches;
  if (boost::regex_search(m_description, matches, descriptionFormatRegex)) {
    constexpr auto preThetaGroup = 1;
    constexpr auto thetaValueGroup = 3;
    m_theta = matches[thetaValueGroup].str();
    m_groupName = matches[preThetaGroup].str();
  } else {
    m_groupName = m_description;
  }
}

const std::string &SearchResult::runNumber() const { return m_runNumber; }

const std::string &SearchResult::description() const { return m_description; }

const std::string &SearchResult::location() const { return m_location; }

const std::string &SearchResult::error() const { return m_error; }

const std::string &SearchResult::groupName() const { return m_groupName; }

const std::string &SearchResult::theta() const { return m_theta; }

void SearchResult::setError(std::string const &error) { m_error = error; }

bool operator==(SearchResult const &lhs, SearchResult const &rhs) {
  // Ignore the error field in the comparison because this represents the
  // state of the item and is not a unique identifier
  return lhs.runNumber() == rhs.runNumber() &&
         lhs.description() == rhs.description() &&
         lhs.location() == rhs.location();
}

bool operator!=(SearchResult const &lhs, SearchResult const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
