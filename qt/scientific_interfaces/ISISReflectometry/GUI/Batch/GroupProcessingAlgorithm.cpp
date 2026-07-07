// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "GroupProcessingAlgorithm.h"

#include <optional>
#include <utility>

#include "../../Reduction/Batch.h"
#include "../../Reduction/Group.h"
#include "../../Reduction/IBatch.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry::GroupProcessing {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
namespace { // unnamed namespace
using namespace Mantid::API;

std::string removePrefix(std::string const &value, std::string const &prefix) {
  // Just return the original value if it doesn't contain the prefix
  if (value.size() <= prefix.size() || value.substr(0, prefix.size()) != prefix)
    return value;

  return value.substr(prefix.size());
}

std::optional<std::string> suffixForWorkspaceGroupMember(std::string const &memberName, std::string const &groupName) {
  auto const prefix = groupName + "_";
  if (memberName.size() <= prefix.size() || memberName.substr(0, prefix.size()) != prefix)
    return std::nullopt;

  return memberName.substr(prefix.size());
}

std::vector<std::string> suffixesFromFirstInputWorkspaceGroup(Group const &group) {
  auto const &ads = AnalysisDataService::Instance();
  for (auto const &row : group.rows()) {
    if (!row)
      continue;

    auto const &groupWSName = row->reducedWorkspaceNames().iVsQ();
    if (!ads.doesExist(groupWSName))
      continue;

    auto const workspaceGroup = ads.retrieveWS<WorkspaceGroup>(groupWSName);
    if (!workspaceGroup)
      continue;

    auto suffixes = std::vector<std::string>();
    for (auto const &memberName : workspaceGroup->getNames()) {
      auto const suffix = suffixForWorkspaceGroupMember(memberName, groupWSName);
      if (!suffix)
        return {};

      suffixes.emplace_back(suffix.value());
    }

    return suffixes;
  }

  return {};
}

void updateWorkspaceProperties(AlgorithmRuntimeProps &properties, Group const &group) {
  // There must be more than workspace to stitch
  if (group.rows().size() < 2)
    throw std::runtime_error("Must have at least two workspaces for stitching");

  // Get the list of input workspaces from the output of each row
  auto workspaces = std::vector<std::string>();
  std::for_each(group.rows().cbegin(), group.rows().cend(), [&workspaces](std::optional<Row> const &row) -> void {
    if (row)
      workspaces.emplace_back(row->reducedWorkspaceNames().iVsQ());
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

  auto const suffixes = suffixesFromFirstInputWorkspaceGroup(group);
  if (!suffixes.empty())
    AlgorithmProperties::update("OutputWorkspaceSuffixes", suffixes, properties);
}

void updateGroupFromOutputProperties(const IAlgorithm_sptr &algorithm, Item &item) {
  auto const stitched = AlgorithmProperties::getOutputWorkspace(algorithm, "OutputWorkspace");
  item.setOutputNames(std::vector<std::string>{stitched});
}

void updateParamsFromResolution(AlgorithmRuntimeProps &properties, std::optional<double> const &resolution) {
  if (!resolution.has_value())
    return;

  // Negate the resolution to give logarithmic binning
  AlgorithmProperties::update("Params", -(resolution.value()), properties);
}

void updateLookupRowProperties(AlgorithmRuntimeProps &properties, std::optional<LookupRow> lookupRow) {
  if (!lookupRow)
    return;

  updateParamsFromResolution(properties, lookupRow->qRange().step());
}

void updateGroupProperties(AlgorithmRuntimeProps &properties, Group const &group) {
  std::optional<double> resolution = std::nullopt;

  for (auto const &row : group.rows()) {
    if (!row.has_value())
      continue;

    // Use the input Q step if provided, or the output Q step otherwise, if set
    if (row->qRange().step().has_value())
      resolution = row->qRange().step();
    else if (row->qRangeOutput().step().has_value())
      resolution = row->qRangeOutput().step();

    // For now just use the first resolution found. Longer term it would be
    // better to check that all rows have the same resolution and set a warning
    // if not.
    if (resolution.has_value())
      break;
  }

  updateParamsFromResolution(properties, resolution);
}

void updateStitchProperties(AlgorithmRuntimeProps &properties,
                            std::map<std::string, std::string> const &stitchParameters) {
  AlgorithmProperties::updateFromMap(properties, stitchParameters);
}

void updateUseValidDataOnly(AlgorithmRuntimeProps &properties) {
  AlgorithmProperties::update("UseValidDataOnly", true, properties);
}
} // unnamed namespace

/** Create a configured algorithm for processing a group. The algorithm
 * properties are set from the reduction configuration model.
 * @param model : the reduction configuration model
 * @param group : the row from the runs table
 */
IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model, Group &group) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create("Stitch1DMany");
  alg->setRethrows(true);

  // Set the algorithm properties from the model
  auto properties = createAlgorithmRuntimeProps(model, group);

  // Return the configured algorithm
  auto jobAlgorithm = std::make_shared<BatchJobAlgorithm>(std::move(alg), std::move(properties),
                                                          updateGroupFromOutputProperties, &group);
  return jobAlgorithm;
}

std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> createAlgorithmRuntimeProps(IBatch const &model,
                                                                                 Group const &group) {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  updateWorkspaceProperties(*properties, group);
  // Set the rebin Params from the lookup row resolution, if given
  updateLookupRowProperties(*properties, model.findWildcardLookupRow());
  // Override the lookup row with the group's rows' resolution,
  // if given
  updateGroupProperties(*properties, group);
  // Override the rebin Params from the user-specified stitch params, if given
  updateStitchProperties(*properties, model.experiment().stitchParameters());
  updateUseValidDataOnly(*properties);
  return properties;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::GroupProcessing
