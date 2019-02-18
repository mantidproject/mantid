// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "GroupProcessingAlgorithm.h"
#include "../../Reduction/Batch.h"
#include "../../Reduction/Group.h"
#include "AlgorithmProperties.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using AlgorithmRuntimeProps = std::map<std::string, std::string>;
namespace { // unnamed namespace

void updateWorkspaceProperties(AlgorithmRuntimeProps &properties,
                               Group const &group) {
  // Get the list of input workspaces from the output of each row
  auto workspaces = std::vector<std::string>();
  std::transform(group.rows().cbegin(), group.rows().cend(),
                 std::back_inserter(workspaces),
                 [](boost::optional<Row> const &row) -> std::string {
                   return row->reducedWorkspaceNames().iVsQ();
                 });
  AlgorithmProperties::update("InputWorkspaces", workspaces, properties);

  // The stitched name is the row output names concatenated but without the
  // individual IvsQ prefixes. Just add one IvsQ prefix at the start.
  auto const prefix = std::string("IvsQ");
  auto outputName = prefix;
  for (auto const &workspace : workspaces) {
    if (workspace.size() <= prefix.size() + 1)
      throw std::runtime_error("Unexpected output workspace name format: " +
                               workspace);
    outputName += "_" + workspace.substr(prefix.size() + 1);
  }
  AlgorithmProperties::update("OutputWorkspace", outputName, properties);
}
} // unnamed namespace

/** Create a configured algorithm for processing a group. The algorithm
 * properties are set from the reduction configuration model.
 * @param model : the reduction configuration model
 * @param group : the row from the runs table
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(Batch const &model,
                                                    Group &group) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create("Stitch1DMany");
  alg->setChild(true);

  // Set the algorithm properties from the model
  auto properties = AlgorithmRuntimeProps();
  updateWorkspaceProperties(properties, group);
  AlgorithmProperties::updateFromMap(properties,
                                     model.experiment().stitchParameters());

  // Store expected output property name
  std::vector<std::string> outputWorkspaceProperties = {"OutputWorkspace"};

  // Return the configured algorithm
  auto jobAlgorithm = boost::make_shared<BatchJobAlgorithm>(
      alg, properties, outputWorkspaceProperties, &group);
  return jobAlgorithm;
}
} // namespace CustomInterfaces
} // namespace MantidQt
