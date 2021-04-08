// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SearchResult.h"
#include "Reduction/ParseReflectometryStrings.h"
#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

SearchResult::SearchResult(const std::string &runNumber, const std::string &title) : m_title(title) {
  parseRun(runNumber);
  parseMetadataFromTitle();
}

SearchResult::SearchResult(const std::string &runNumber, const std::string &title, const std::string &groupName,
                           const std::string &theta, const std::string &error, const std::string &excludeReason,
                           const std::string &comment)
    : m_runNumber(runNumber), m_title(title), m_groupName(groupName), m_theta(theta), m_error(error),
      m_excludeReason(excludeReason), m_comment(comment) {}

void SearchResult::parseRun(std::string const &runNumber) {
  auto const maybeRunNumber = parseRunNumber(runNumber);
  if (maybeRunNumber.is_initialized())
    m_runNumber = maybeRunNumber.get();
  else
    addError("Run number is not specified");
}

/** Extract the group name and angle from the run title. Expects the title to
 * be in the format: "group_name th=angle". If it is not in this format, then
 * the group name is set to the full title, the angle is empty, and the error
 * message is set.
 */
void SearchResult::parseMetadataFromTitle() {
  static boost::regex titleFormatRegex("(.*)(th[:=]\\s*([0-9.\\-]+))(.*)");
  boost::smatch matches;

  if (!boost::regex_search(m_title, matches, titleFormatRegex)) {
    m_groupName = m_title;
    addError("Theta was not specified in the run title.");
    return;
  }

  constexpr auto preThetaGroup = 1;
  constexpr auto thetaValueGroup = 3;
  m_theta = matches[thetaValueGroup].str();
  m_groupName = matches[preThetaGroup].str();

  // Validate that the angle parses correctly
  auto const maybeTheta = parseTheta(m_theta);
  if (!maybeTheta.is_initialized())
    addError(std::string("Invalid theta value in run title: ").append(m_theta));
}

const std::string &SearchResult::runNumber() const { return m_runNumber; }

const std::string &SearchResult::title() const { return m_title; }

const std::string &SearchResult::error() const { return m_error; }

const std::string &SearchResult::groupName() const { return m_groupName; }

const std::string &SearchResult::theta() const { return m_theta; }

bool SearchResult::hasError() const { return !m_error.empty(); }

bool SearchResult::exclude() const { return !m_excludeReason.empty(); }

const std::string &SearchResult::excludeReason() const { return m_excludeReason; }

bool SearchResult::hasComment() const { return !m_comment.empty(); }

const std::string &SearchResult::comment() const { return m_comment; }

void SearchResult::addError(std::string const &error) {
  if (m_error.empty())
    m_error = error;
  else
    m_error.append("\n").append(error);
}

void SearchResult::addExcludeReason(std::string const &excludeReason) { m_excludeReason = excludeReason; }

void SearchResult::addComment(std::string const &comment) { m_comment = comment; }

bool operator==(SearchResult const &lhs, SearchResult const &rhs) {
  // Ignore the error field in the comparison because this represents the
  // state of the item and is not a unique identifier
  return lhs.runNumber() == rhs.runNumber() && lhs.title() == rhs.title();
}

bool operator!=(SearchResult const &lhs, SearchResult const &rhs) { return !(lhs == rhs); }
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
