// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunWorkspaceResolver.h"
#include "Common/GroupHelper.h"
#include "Common/Parse.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include <algorithm>
#include <exception>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
namespace {
auto members(std::string const &name) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  auto result = ads.doesExist(name) ? getMembers(ads.retrieve(name)) : std::vector<Mantid::API::MatrixWorkspace_sptr>{};
  std::erase(result, nullptr);
  return result;
}

std::string runNumber(std::string const &name) {
  for (auto const &workspace : members(name))
    if (workspace->run().hasProperty("run_number"))
      if (auto value = workspace->run().getProperty("run_number")->value(); !value.empty())
        return value;
  return "";
}

} // namespace

Mantid::API::MatrixWorkspace_sptr findInputWorkspace(std::vector<std::string> const &inputs, std::optional<int> period,
                                                     std::vector<std::string> const &reducedWorkspaces) {
  if (inputs.empty())
    return nullptr;

  // Completed reductions provide the authoritative run identity.
  std::string expectedRun;
  for (auto const &name : reducedWorkspaces)
    if (expectedRun = runNumber(name); !expectedRun.empty())
      break;

  if (expectedRun.empty())
    return nullptr;

  std::vector<std::string> candidates;
  if (inputs.size() == 1)
    candidates = {inputs.front(), "TOF_" + inputs.front(),
                  "__TOF_" + inputs.front()}; // if only single run, these are valid candidates
  candidates.insert(candidates.end(), {"TOF_" + expectedRun, "__TOF_" + expectedRun, "TOF", "__TOF"});
  for (auto const &name : candidates) {
    for (auto const &workspace : members(name)) {
      auto const &run = workspace->run();
      if (!run.hasProperty("run_number"))
        continue;
      auto const number = run.getProperty("run_number")->value();
      if (number.empty())
        continue;
      if (number != expectedRun) {
        auto const expectedNumber = parseNonNegativeInt(expectedRun);
        if (!parseNonNegativeInt(number) || !expectedNumber || workspace->getRunNumber() != *expectedNumber)
          continue;
      }
      try {
        // If a period is specified, check that the workspace has the correct period metadata.
        if (!period ||
            (run.hasProperty("current_period") && run.getPropertyAsIntegerValue("current_period") == *period))
          return workspace;
      } catch (std::exception const &) {
        // Ignore a workspace with invalid period metadata.
      }
    }
  }
  return nullptr;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
