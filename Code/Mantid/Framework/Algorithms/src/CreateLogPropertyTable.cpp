//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreateLogPropertyTable.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include "boost/shared_ptr.hpp"

#include <vector>
#include <map>
#include <assert.h>

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
}

/**
 * Initialise the algorithm's properties.
 */
void CreateLogPropertyTable::init() {
  // Input workspaces
  declareProperty(
      new ArrayProperty<std::string>(
          "InputWorkspaces",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "Name of the Input Workspaces from which to get log properties.");

  // Output workspace
  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of the output ITableWorkspace.");

  // Which log properties to use
  declareProperty(
      new ArrayProperty<std::string>(
          "LogPropertyNames",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "The names of the log properties to place in table.");

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
  for (auto matrixWs = matrixWsList.begin(); matrixWs != matrixWsList.end();
       ++matrixWs) {
    const Run &run = matrixWs->get()->run();
    const std::string wsName = matrixWs->get()->getName();

    // Throw if a run does not have a property.
    for (auto propName = propNames.begin(); propName != propNames.end();
         ++propName)
      if (!run.hasProperty(*propName))
        throw std::runtime_error("\"" + wsName +
                                 "\" does not have a run property of \"" +
                                 *propName + "\".");
  }

  // Set up output table.
  boost::shared_ptr<ITableWorkspace> outputTable =
      WorkspaceFactory::Instance().createTable();
  // One column for each property.
  for (auto propName = propNames.begin(); propName != propNames.end();
       ++propName)
    outputTable->addColumn("str", *propName);
  // One row for each workspace.
  for (size_t i = 0; i < matrixWsList.size(); ++i)
    outputTable->appendRow();

  // Change all "plot designation" fields to "None". (This basically means that
  // column headings
  // appear as, for example, "inst_abrv" instead of "inst_abrv [X]" or similar.)
  for (size_t i = 0; i < outputTable->columnCount(); ++i)
    outputTable->getColumn(i)->setPlotType(0);

  // Populate output table with the requested run properties.
  for (size_t i = 0; i < outputTable->rowCount(); ++i) {
    TableRow row = outputTable->getRow(i);
    MatrixWorkspace_sptr matrixWs = matrixWsList[i];

    for (auto propName = propNames.begin(); propName != propNames.end();
         ++propName) {
      const std::string propValue =
          matrixWs->run().getProperty(*propName)->value();
      row << propValue;
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
  for (auto wsName = wsNames.begin(); wsName != wsNames.end(); ++wsName) {
    WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(
        AnalysisDataService::Instance().retrieve(*wsName));
    MatrixWorkspace_sptr matrixWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(*wsName));

    if (wsGroup) {
      const std::vector<std::string> childNames = wsGroup->getNames();

      // If there are no child workspaces in the group (is this possible?), just
      // ignore it.
      if (childNames.empty())
        break;

      // Retrieve pointers to all the child workspaces.
      std::vector<MatrixWorkspace_sptr> childWsList;
      for (auto childName = childNames.begin(); childName != childNames.end();
           ++childName) {
        childWsList.push_back(
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                *childName));
      }

      // Deal with child workspaces according to policy.
      switch (groupPolicy) {
      case ALL: {
        // Append all the children to the list.
        for (auto childWs = childWsList.begin(); childWs != childWsList.end();
             ++childWs)
          matrixWsList.push_back(*childWs);
        break;
      }
      case FIRST:
        // Append only the first child to the list.
        matrixWsList.push_back(childWsList[0]);
        break;
      case NONE:
        // Add nothing to the list.
        break;
      default:
        // We should never reach here.
        assert(false);
      }
    } else if (matrixWs) {
      matrixWsList.push_back(matrixWs);
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
    map.insert(std::make_pair("All", ALL));
    map.insert(std::make_pair("First", FIRST));
    map.insert(std::make_pair("None", NONE));
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

  for (auto policy = map.begin(); policy != map.end(); ++policy)
    groupPolicyNames.insert(policy->first);

  return groupPolicyNames;
}
}

} // namespace Algorithms
} // namespace Mantid
