// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateLogPropertyTable.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include "boost/shared_ptr.hpp"

#include <cassert>
#include <map>
#include <vector>

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateLogPropertyTable)

using namespace Kernel;
using namespace API;

namespace {
enum GroupPolicy { ALL, FIRST, NONE };

// Forward declarations.
std::vector<MatrixWorkspace_sptr>
retrieveMatrixWsList(const std::vector<std::string> &wsNames,
                     GroupPolicy groupPolicy);
GroupPolicy getGroupPolicyByName(const std::string &name);
std::set<std::string> getAllGroupPolicyNames();
Math::StatisticType getStatisticTypeByName(const std::string &name);
std::set<std::string> getAllStatisticTypeNames();
} // namespace

/**
 * Initialise the algorithm's properties.
 */
void CreateLogPropertyTable::init() {
  // Input workspaces
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "InputWorkspaces",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "Name of the Input Workspaces from which to get log properties.");

  // Output workspace
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output ITableWorkspace.");

  // Which log properties to use
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "LogPropertyNames",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "The names of the log properties to place in table.");

  // How to handle time series logs
  const std::set<std::string> statisticNames = getAllStatisticTypeNames();
  declareProperty("TimeSeriesStatistic", "Mean",
                  boost::make_shared<StringListValidator>(statisticNames),
                  "The statistic to use when adding a time series log.");

  // How to handle workspace groups
  const std::set<std::string> groupPolicies = getAllGroupPolicyNames();
  declareProperty("GroupPolicy", "First",
                  boost::make_shared<StringListValidator>(groupPolicies),
                  "The policy by which to handle GroupWorkspaces.  \"All\" "
                  "will include all children in the table, \"First\" will "
                  "include "
                  "the first child, and \"None\" will not include any.");
}

/**
 * Execute the algorithm.
 */
void CreateLogPropertyTable::exec() {
  std::vector<std::string> wsNames = this->getProperty("InputWorkspaces");

  // Retrieve a list of MatrixWorkspace pointers, using the given "GroupPolicy".
  const std::string groupPolicyName = this->getPropertyValue("GroupPolicy");
  const GroupPolicy groupPolicy = getGroupPolicyByName(groupPolicyName);
  const std::vector<MatrixWorkspace_sptr> matrixWsList =
      retrieveMatrixWsList(wsNames, groupPolicy);

  // Get the names of the properties that will be stored.
  const std::vector<std::string> propNames =
      this->getProperty("LogPropertyNames");

  // Make sure all workspaces contain the properties.
  for (const auto &matrixWs : matrixWsList) {
    const Run &run = matrixWs->run();
    const std::string wsName = matrixWs->getName();

    // Throw if a run does not have a property.
    for (const auto &propName : propNames)
      if (!run.hasProperty(propName))
        throw std::runtime_error("\"" + wsName +
                                 "\" does not have a run property of \"" +
                                 propName + "\".");
  }

  // Set up output table.
  auto outputTable = boost::make_shared<DataObjects::TableWorkspace>();
  // One column for each property.
  for (const auto &propName : propNames)
    outputTable->addColumn("str", propName);
  // One row for each workspace.
  for (size_t i = 0; i < matrixWsList.size(); ++i)
    outputTable->appendRow();

  // Set the first column to X and all others to Y
  // This is to reduce the number of steps required to plot the data
  for (size_t i = 0; i < outputTable->columnCount(); ++i)
    outputTable->getColumn(i)->setPlotType(i == 0 ? 1 : 2);

  const std::string timeSeriesStatName =
      this->getPropertyValue("TimeSeriesStatistic");
  const Math::StatisticType timeSeriesStat =
      getStatisticTypeByName(timeSeriesStatName);
  // Populate output table with the requested run properties.
  for (size_t i = 0; i < outputTable->rowCount(); ++i) {
    TableRow row = outputTable->getRow(i);
    MatrixWorkspace_sptr matrixWs = matrixWsList[i];

    for (const auto &propName : propNames) {
      Property *prop = matrixWs->run().getProperty(propName);
      std::stringstream propValue;

      if (prop->type().find("TimeValue") != std::string::npos) {
        propValue << matrixWs->run().getLogAsSingleValue(propName,
                                                         timeSeriesStat);
      } else {
        propValue << prop->value();
      }

      row << propValue.str();
    }
  }

  this->setProperty("OutputWorkspace", outputTable);
}

namespace {
/**
 * Given a list of workspace names, will retrieve pointers to the corresponding
 *workspaces in the ADS.
 * Only MatrixWorkspaces or the children of groups of MatrixWorkspaces are
 *retrieved. GroupWorkspaces
 * are dealt with according to m_groupPolicy:
 *
 * "All"   - Retrieve pointers to all the children of a group.
 * "First" - Only retrieve a pointer to the first child of a group.
 * "None"  - No pointers are retreived.
 *
 * @param wsNames     :: the list of workspaces to retrieve pointers to.
 * @param groupPolicy :: the policy by which to deal with group workspaces.
 *
 * @return the retrieved MatrixWorkspace pointers
 */
std::vector<MatrixWorkspace_sptr>
retrieveMatrixWsList(const std::vector<std::string> &wsNames,
                     GroupPolicy groupPolicy) {
  std::vector<MatrixWorkspace_sptr> matrixWsList;

  // Get all the workspaces which are to be inspected for log proeprties.
  auto &ADS = AnalysisDataService::Instance();
  for (const auto &wsName : wsNames) {

    WorkspaceGroup_sptr wsGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(ADS.retrieve(wsName));
    MatrixWorkspace_sptr matrixWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(ADS.retrieve(wsName));

    if (wsGroup) {
      const std::vector<std::string> childNames = wsGroup->getNames();

      // If there are no child workspaces in the group (is this possible?), just
      // ignore it.
      if (childNames.empty())
        break;

      // Retrieve pointers to all the child workspaces.
      std::vector<MatrixWorkspace_sptr> childWsList;
      childWsList.reserve(childNames.size());
      for (const auto &childName : childNames) {
        childWsList.emplace_back(ADS.retrieveWS<MatrixWorkspace>(childName));
      }

      // Deal with child workspaces according to policy.
      switch (groupPolicy) {
      case ALL: {
        // Append all the children to the list.
        std::move(childWsList.cbegin(), childWsList.cend(),
                  std::back_inserter(matrixWsList));
        break;
      }
      case FIRST:
        // Append only the first child to the list.
        matrixWsList.emplace_back(childWsList[0]);
        break;
      case NONE:
        // Add nothing to the list.
        break;
      default:
        // We should never reach here.
        assert(false);
      }
    } else if (matrixWs) {
      matrixWsList.emplace_back(matrixWs);
    }
  }

  return matrixWsList;
}

/**
 * Returns a constant reference to a static map, which maps group policy
 * names to actual GroupPolicy enum members.
 *
 * @returns map of group policy names to GroupPolicy enum members.
 */
const std::map<std::string, GroupPolicy> &getGroupPolicyMap() {
  static std::map<std::string, GroupPolicy> map;

  // Populate the map if empty.
  if (map.empty()) {
    map.emplace("All", ALL);
    map.emplace("First", FIRST);
    map.emplace("None", NONE);
  }

  return map;
}

/**
 * Given a group policy name, will return the corresponding GroupPolicy enum
 *member.
 *
 * @param name :: name of group policy.
 *
 * @returns the corresponding GroupPolicy enum member.
 */
GroupPolicy getGroupPolicyByName(const std::string &name) {
  const std::map<std::string, GroupPolicy> &map = getGroupPolicyMap();

  // If we can find a policy with the given name, return it.
  auto policy = map.find(name);
  if (policy != map.end())
    return policy->second;

  // Else return ALL as default.  Assert since we should never reach here.
  assert(false);
  return ALL;
}

/**
 * Returns a set of all group policy names.
 *
 * @returns a set of all group policy names.
 */
std::set<std::string> getAllGroupPolicyNames() {
  const std::map<std::string, GroupPolicy> &map = getGroupPolicyMap();
  std::set<std::string> groupPolicyNames;

  for (const auto &policy : map)
    groupPolicyNames.insert(policy.first);

  return groupPolicyNames;
}

/**
 * Returns a constant reference to a static map, which maps statistic
 * names to Kernel::Math::StatisticType members.
 *
 * @returns map of statistic names and StatisticType members
 */
const std::map<std::string, Math::StatisticType> &getStatisticTypeMap() {
  static std::map<std::string, Math::StatisticType> map;

  // Populate the map if empty.
  if (map.empty()) {
    map.emplace("FirstValue", Math::StatisticType::FirstValue);
    map.emplace("LastValue", Math::StatisticType::LastValue);
    map.emplace("Minimum", Math::StatisticType::Minimum);
    map.emplace("Maximum", Math::StatisticType::Maximum);
    map.emplace("Mean", Math::StatisticType::Mean);
    map.emplace("Median", Math::StatisticType::Median);
  }

  return map;
}

/**
 * Given a statistic type name, will return the corresponding StatisticType.
 *
 * @param name :: name of statistic
 * @returns StatisticType
 */
Math::StatisticType getStatisticTypeByName(const std::string &name) {
  const std::map<std::string, Math::StatisticType> &map = getStatisticTypeMap();

  // If we can find a policy with the given name, return it.
  auto policy = map.find(name);
  if (policy != map.end())
    return policy->second;

  // Else return ALL as default.  Assert since we should never reach here.
  return Math::StatisticType::Mean;
}

/**
 * Returns a set of all statistic type names.
 *
 * @returns a set of all statistic type names.
 */
std::set<std::string> getAllStatisticTypeNames() {
  const std::map<std::string, Math::StatisticType> &map = getStatisticTypeMap();
  std::set<std::string> statisticTypeNames;

  for (const auto &policy : map)
    statisticTypeNames.insert(policy.first);

  return statisticTypeNames;
}
} // namespace
} // namespace Algorithms
} // namespace Mantid
