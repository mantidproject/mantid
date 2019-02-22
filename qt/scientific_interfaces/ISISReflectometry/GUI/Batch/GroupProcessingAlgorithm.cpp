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

std::string removePrefix(std::string const &value, std::string const &prefix) {
  // Just return the original value if it doesn't contain the prefix
  if (value.size() <= prefix.size() || value.substr(0, prefix.size()) != prefix)
    return value;

  return value.substr(prefix.size());
}

void updateWorkspaceProperties(AlgorithmRuntimeProps &properties,
                               Group const &group) {
  // There must be more than workspace to stitch
  if (group.rows().size() < 2)
    throw std::runtime_error("Must have at least two workspaces for stitching");

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
  auto outputName = std::string("IvsQ");
  auto const prefix = std::string("IvsQ_");
  auto const separator = std::string("_");
  for (auto const &workspace : workspaces) {
    auto name = removePrefix(workspace, prefix);
    outputName += separator + name;
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
  alg->setRethrows(true);

  // Set the algorithm properties from the model
  auto properties = createAlgorithmRuntimeProps(model, group);

  // Store expected output property name
  std::vector<std::string> outputWorkspaceProperties = {"OutputWorkspace"};

  // Return the configured algorithm
  auto jobAlgorithm = boost::make_shared<BatchJobAlgorithm>(
      alg, properties, outputWorkspaceProperties, &group);
  return jobAlgorithm;
}

AlgorithmRuntimeProps createAlgorithmRuntimeProps(Batch const &model,
                                                  Group const &group) {
  auto properties = AlgorithmRuntimeProps();
  updateWorkspaceProperties(properties, group);
  AlgorithmProperties::updateFromMap(properties,
                                     model.experiment().stitchParameters());
  return properties;
}
} // namespace CustomInterfaces
} // namespace MantidQt
