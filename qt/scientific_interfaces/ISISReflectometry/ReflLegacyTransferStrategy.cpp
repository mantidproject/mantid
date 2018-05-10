#include "ReflLegacyTransferStrategy.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Tolerance.h"
#include "ReflTableSchema.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {
/** Check that the given row contains a valid theta value
 * @throws : std::invalid_argument if failed
 */
void validateTheta(std::map<std::string, std::string> row) {
  // Exclude empty strings
  auto const angleString = row[ReflTableSchema::ANGLE];
  if (angleString.empty())
    throw std::invalid_argument("Theta is not specified");

  // Exclude zero or negative angles
  try {
    auto const angle = std::stod(angleString);
    if (angle < Mantid::Kernel::Tolerance)
      throw std::invalid_argument("Theta is zero or negative");
  } catch (const std::exception &e) {
    throw std::invalid_argument(std::string("Error parsing Theta: ") +
                                e.what());
  }
}

/** Check that the given row values are all valid
 * @throws : std::invalid_argument if failed
 */
void validateStrict(std::map<std::string, std::string> row) {
  validateTheta(row);
}

/** Check that the given row contains valid data
 * @throws : std::invalid_argument if failed
 */
void validateRow(std::map<std::string, std::string> row,
                 const TransferMatch matchType) {
  switch (matchType) {
  case TransferMatch::Any:
    // no checks required
    break;
  case TransferMatch::ValidTheta:
    validateTheta(row);
    break;
  case TransferMatch::Strict:
    validateStrict(row);
    break;
  };
}
}

TransferResults
ReflLegacyTransferStrategy::transferRuns(SearchResultMap &searchResults,
                                         Mantid::Kernel::ProgressBase &progress,
                                         const TransferMatch matchType) {
  /*
   * If the descriptions are the same except for theta: same group, different
   * rows.
   * If the descriptions are the same including theta: same row with runs
   * separated by '+'
   * We always prefill theta if we can.
   */

  // maps descriptions to runs. Multiple runs are joined with '+'
  std::map<std::string, std::string> descriptionToRun;
  // maps a description to a group. If descriptions only differ by theta,
  // they'll share a group
  std::map<std::string, std::string> descriptionToGroup;
  // maps descriptions to the value of theta they contain
  std::map<std::string, std::string> descriptionToTheta;

  // Iterate over the input and build the maps
  for (auto const &runDescriptionPair : searchResults) {
    const auto run = runDescriptionPair.first;
    const auto description = runDescriptionPair.second.description;
    auto groupName = description;
    auto cleanDescription = description;

    static boost::regex descriptionFormatRegex("(.*)(th[:=]([0-9.]+))(.*)");
    boost::smatch matches;
    if (boost::regex_search(description, matches, descriptionFormatRegex)) {
      constexpr auto preThetaGroup = 1;
      constexpr auto thetaValueGroup = 3;
      constexpr auto postThetaGroup = 4;
      // We have theta. Let's get a clean description
      const auto theta = matches[thetaValueGroup].str();
      const auto preTheta = matches[preThetaGroup].str();
      const auto postTheta = matches[postThetaGroup].str();
      groupName = preTheta;
      cleanDescription = preTheta + "?" + postTheta;
      descriptionToTheta[description] = theta;
    }

    // map the description to the run, making sure to join with a + if one
    // already exists
    const std::string prevRun = descriptionToRun[description];
    if (prevRun.empty())
      descriptionToRun[description] = run;
    else
      descriptionToRun[description] = prevRun + "+" + run;

    // If there isn't a group for this description (ignoring differences in
    // theta) yet, make one
    if (descriptionToGroup[cleanDescription].empty()) {
      boost::trim(groupName);
      descriptionToGroup[cleanDescription] = groupName;
    }

    // Assign this description to the group it belongs to
    descriptionToGroup[description] = descriptionToGroup[cleanDescription];

    progress.report();
  }

  // All the data we need is now properly organised, so we can quickly throw out
  // the rows needed
  std::vector<std::map<std::string, std::string>> rows;
  std::vector<std::map<std::string, std::string>>
      errors; // will remain empty for now
  TransferResults results(rows, errors);
  for (const auto &runDescriptionPair : descriptionToRun) {
    // set up our successful run into table-ready format.
    std::map<std::string, std::string> row;
    row[ReflTableSchema::RUNS] = runDescriptionPair.second;
    row[ReflTableSchema::ANGLE] = descriptionToTheta[runDescriptionPair.first];
    row[ReflTableSchema::GROUP] = descriptionToGroup[runDescriptionPair.first];
    try {
      validateRow(row, matchType);
    } catch (const std::exception &e) {
      results.addErrorRow(row[ReflTableSchema::RUNS], e.what());
      continue;
    }
    // add our successful row
    results.addTransferRow(row);
  }
  std::sort(results.m_transferRuns.begin(), results.m_transferRuns.end());
  return results;
}

ReflLegacyTransferStrategy *ReflLegacyTransferStrategy::doClone() const {
  return new ReflLegacyTransferStrategy(*this);
}

std::unique_ptr<ReflLegacyTransferStrategy>
ReflLegacyTransferStrategy::clone() const {
  return std::unique_ptr<ReflLegacyTransferStrategy>(doClone());
}

bool MantidQt::CustomInterfaces::ReflLegacyTransferStrategy::knownFileType(
    const std::string &filename) const {
  boost::regex pattern("raw$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}
}
}
