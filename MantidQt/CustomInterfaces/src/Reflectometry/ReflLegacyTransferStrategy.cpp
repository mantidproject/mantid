#include "MantidQtCustomInterfaces/Reflectometry/ReflLegacyTransferStrategy.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {
TransferResults ReflLegacyTransferStrategy::transferRuns(
    SearchResultMap &searchResults, Mantid::Kernel::ProgressBase &progress) {
  /*
   * If the descriptions are the same except for theta: same group, different
   * rows.
   * If the descriptions are the same including theta: same row with runs
   * separated by '+'
   * We always prefill theta if we can.
   */

  // maps descriptions to runs. Multiple runs are joined with '+'
  std::map<std::string, std::string> runsByDesc;
  // maps a description to a group. If descriptions only differ by theta,
  // they'll share a group
  std::map<std::string, std::string> groupsByDesc;
  // maps descriptions to the value of theta they contain
  std::map<std::string, std::string> thetaByDesc;

  // Iterate over the input and build the maps
  for (auto rowIt = searchResults.begin(); rowIt != searchResults.end();
       ++rowIt) {
    const std::string run = rowIt->first;
    const std::string desc = rowIt->second.description;
    std::string cleanDesc = desc;

    // See if theta is in the description
    static boost::regex regexTheta(
        "(?|th[:=](?<theta>[0-9.]+)|in (?<theta>[0-9.]+) theta)");
    boost::smatch matches;
    if (boost::regex_search(desc, matches, regexTheta)) {
      // We have theta. Let's get a clean description
      size_t matchOffset = matches.position("theta");
      const std::string theta = matches["theta"].str();
      const std::string descPreTheta = desc.substr(0, matchOffset);
      const std::string descPostTheta =
          desc.substr(matchOffset + theta.length(), std::string::npos);
      cleanDesc = descPreTheta + "?" + descPostTheta;
      thetaByDesc[desc] = theta;
    }

    // map the description to the run, making sure to join with a + if one
    // already exists
    const std::string prevRun = runsByDesc[desc];
    if (prevRun.empty())
      runsByDesc[desc] = run;
    else
      runsByDesc[desc] = prevRun + "+" + run;

    // If there isn't a group for this description (ignoring differences in
    // theta) yet, make one
    if (groupsByDesc[cleanDesc].empty())
      groupsByDesc[cleanDesc] = desc.substr(0, desc.find("th") - 1);

    // Assign this description to the group it belongs to
    groupsByDesc[desc] = groupsByDesc[cleanDesc];

    progress.report();
  }

  // All the data we need is now properly organised, so we can quickly throw out
  // the rows needed
  std::vector<std::map<std::string, std::string>> rows;
  std::vector<std::map<std::string, std::string>>
      errors; // will remain empty for now
  TransferResults results(rows, errors);
  for (auto run = runsByDesc.begin(); run != runsByDesc.end(); ++run) {
    // set up our successful run into table-ready format.
    std::map<std::string, std::string> row;
    row[ReflTableSchema::RUNS] = run->second;
    row[ReflTableSchema::ANGLE] = thetaByDesc[run->first];
    row[ReflTableSchema::GROUP] = groupsByDesc[run->first];
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
