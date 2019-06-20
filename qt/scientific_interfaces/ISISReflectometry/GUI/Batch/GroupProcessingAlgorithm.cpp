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
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
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
  std::for_each(group.rows().cbegin(), group.rows().cend(),
                [&workspaces](boost::optional<Row> const &row) -> void {
                  if (row)
                    workspaces.push_back(row->reducedWorkspaceNames().iVsQ());
                });
  AlgorithmProperties::update("InputWorkspaces", workspaces, properties);

  // The stitched name is the row output names concatenated but without the
  // individual IvsQ prefixes. Just add one IvsQ prefix at the start.
  auto outputName = std::string("IvsQ");
  auto const prefix = std::string("IvsQ_");
  auto const separator = std::string("_");
  for (auto const &workspace : workspaces) {
    auto const name = removePrefix(workspace, prefix);
    outputName += separator + name;
  }
  AlgorithmProperties::update("OutputWorkspace", outputName, properties);
}

void updateGroupFromOutputProperties(IAlgorithm_sptr algorithm, Item &group) {
  auto const stitched =
      AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspace");
  group.setOutputNames(std::vector<std::string>{stitched});
}

void updateParamsFromResolution(AlgorithmRuntimeProps &properties,
                                boost::optional<double> const &resolution) {
  if (!resolution.is_initialized())
    return;

  // Negate the resolution to give logarithmic binning
  AlgorithmProperties::update("Params", -(resolution.get()), properties);
}

void updatePerThetaDefaultProperties(AlgorithmRuntimeProps &properties,
                                     PerThetaDefaults const *perThetaDefaults) {
  if (!perThetaDefaults)
    return;

  updateParamsFromResolution(properties, perThetaDefaults->qRange().step());
}

void updateGroupProperties(AlgorithmRuntimeProps &properties,
                           Group const &group) {
  auto resolution = boost::optional<double>(boost::none);

  for (auto const &row : group.rows()) {
    if (!row.is_initialized())
      continue;

    // Use the input Q step if provided, or the output Q step otherwise, if set
    if (row->qRange().step().is_initialized())
      resolution = row->qRange().step();
    else if (row->qRangeOutput().step().is_initialized())
      resolution = row->qRangeOutput().step();

    // For now just use the first resolution found. Longer term it would be
    // better to check that all rows have the same resolution and set a warning
    // if not.
    if (resolution.is_initialized())
      break;
  }

  updateParamsFromResolution(properties, resolution);
}

void updateStitchProperties(
    AlgorithmRuntimeProps &properties,
    std::map<std::string, std::string> const &stitchParameters) {
  AlgorithmProperties::updateFromMap(properties, stitchParameters);
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

  // Return the configured algorithm
  auto jobAlgorithm = boost::make_shared<BatchJobAlgorithm>(
      alg, properties, updateGroupFromOutputProperties, &group);
  return jobAlgorithm;
}

AlgorithmRuntimeProps createAlgorithmRuntimeProps(Batch const &model,
                                                  Group const &group) {
  auto properties = AlgorithmRuntimeProps();
  updateWorkspaceProperties(properties, group);
  // Set the rebin Params from the per theta defaults resolution, if given
  updatePerThetaDefaultProperties(properties, model.wildcardDefaults());
  // Override the per theta defaults params with the group's rows' resolution,
  // if given
  updateGroupProperties(properties, group);
  // Override the rebin Params from the user-specified stitch params, if given
  updateStitchProperties(properties, model.experiment().stitchParameters());
  return properties;
}
} // namespace CustomInterfaces
} // namespace MantidQt
