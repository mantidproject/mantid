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

std::optional<std::pair<std::string, Mantid::API::MatrixWorkspace_sptr>>
getFirstValidRun(std::string const &name, const std::optional<int> period,
                 const std::optional<std::string> expectedRunNumber) {
  for (auto const &workspace : members(name)) {
    auto const &run = workspace->run();
    if (!run.hasProperty("run_number"))
      continue;

    if (period) {
      if (!run.hasProperty("current_period"))
        continue;
      auto const currentPeriod = parseNonNegativeInt(run.getProperty("current_period")->value());
      if (currentPeriod != period)
        continue;
    }

    if (auto const &value = run.getProperty("run_number")->value(); !value.empty()) {
      if (expectedRunNumber && value != *expectedRunNumber) {
        auto const expectedNumber = parseNonNegativeInt(*expectedRunNumber);
        if (!expectedNumber || parseNonNegativeInt(value) != expectedNumber)
          continue;
      }
      return std::pair{value, workspace};
    }
  }
  return std::nullopt;
}

} // namespace

Mantid::API::MatrixWorkspace_sptr findInputWorkspace(std::vector<std::string> const &inputs, std::optional<int> period,
                                                     std::vector<std::string> const &reducedWorkspaces) {
  if (inputs.empty())
    return nullptr;

  // Completed reductions provide the authoritative run identity.
  std::string expectedRun;
  for (auto const &name : reducedWorkspaces)
    if (auto const &validRun = getFirstValidRun(name, std::nullopt, std::nullopt); validRun) {
      expectedRun = validRun.value().first;
      break;
    }
  if (expectedRun.empty())
    return nullptr;

  std::vector<std::string> candidates;
  if (inputs.size() == 1)
    candidates = {inputs.front(), "TOF_" + inputs.front(),
                  "__TOF_" + inputs.front()}; // if only single run, these are valid candidates
  candidates.insert(candidates.end(), {"TOF_" + expectedRun, "__TOF_" + expectedRun, "TOF", "__TOF"});
  for (auto const &name : candidates) {
    if (auto const &validRun = getFirstValidRun(name, period, expectedRun); validRun) {
      return validRun.value().second;
    }
  }
  return nullptr;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
