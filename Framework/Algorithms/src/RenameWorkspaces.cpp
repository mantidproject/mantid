// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RenameWorkspaces.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RenameWorkspaces)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void RenameWorkspaces::init() {
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "InputWorkspaces",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "Names of the Input Workspaces");
  // WorkspaceNames - List of new names
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "WorkspaceNames", Direction::Input),
                  "New Names of the Workspaces");
  // --or--
  // Prefix
  declareProperty("Prefix", std::string(""),
                  "Prefix to add to input workspace names", Direction::Input);
  // Suffix
  declareProperty("Suffix", std::string(""),
                  "Suffix to add to input workspace names", Direction::Input);
  // Set to default true to maintain compatibility with existing scripts
  // as this just allowed overriding by default
  declareProperty<bool>(
      "OverwriteExisting", true,
      "If true all existing workspaces with the output name will be"
      " overwritten. Defaults to true to maintain backwards compatibility.",
      Direction::Input);
}

/**
 * Validates that the output names do not already exist
 * @return A map of the workspace property and error message
 */
std::map<std::string, std::string> RenameWorkspaces::validateInputs() {
  using namespace std;
  map<string, string> errorList;

  // Get the workspace name list
  std::vector<std::string> newWsName = getProperty("WorkspaceNames");
  // Get the prefix and suffix
  std::string prefix = getPropertyValue("Prefix");
  std::string suffix = getPropertyValue("Suffix");

  // Check properties
  if (newWsName.empty() && prefix.empty() && suffix.empty()) {
    errorList["WorkspaceNames"] =
        "No list of Workspace names, prefix or suffix has been supplied.";
  }

  if (!newWsName.empty() && (!prefix.empty() || !suffix.empty())) {
    errorList["WorkspaceNames"] = "Both a list of workspace names and a prefix "
                                  "or suffix has been supplied.";
    if (!prefix.empty()) {
      errorList["Prefix"] = "Both a list of workspace names and a prefix "
                            "or suffix has been supplied.";
    } else {
      errorList["Suffix"] = "Both a list of workspace names and a prefix "
                            "or suffix has been supplied.";
    }
  }

  if (newWsName.size() > 1) {
    for (size_t i = 0; i < newWsName.size() - 1; ++i) {
      for (size_t j = i + 1; j < newWsName.size(); ++j) {
        if (newWsName[i] == newWsName[j]) {
          errorList["WorkspaceNames"] =
              "Duplicate '" + newWsName[i] + "' found in WorkspaceNames.";
        }
      }
    }
  }

  return errorList;
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void RenameWorkspaces::exec() {
  // Get the input workspace list
  std::vector<std::string> inputWsName = getProperty("InputWorkspaces");

  // Get the workspace name list
  std::vector<std::string> newWsName = getProperty("WorkspaceNames");
  // Get the prefix and suffix
  std::string prefix = getPropertyValue("Prefix");
  std::string suffix = getPropertyValue("Suffix");

  bool overrideWorkspace = getProperty("OverwriteExisting");

  // Check properties

  size_t nWs = inputWsName.size();
  if (!newWsName.empty()) {
    // We are using a list of new names
    if (nWs > newWsName.size()) {
      nWs = newWsName.size();
      // could issue warning here
    }
  } else { // We are using prefix and/or suffix
    // Build new names.
    for (size_t i = 0; i < nWs; ++i) {
      newWsName.push_back(prefix + inputWsName[i] + suffix);
    }
  }

  // Check all names are not used already before starting rename
  for (const auto &name : newWsName) {
    if (AnalysisDataService::Instance().doesExist(name)) {
      // Name exists, stop if override if not set but let
      // RenameWorkspace handle deleting if we are overriding
      if (!overrideWorkspace) {
        throw std::runtime_error("A workspace called " + name +
                                 " already exists");
      }
    }
  }

  // loop over array and rename each workspace
  for (size_t i = 0; i < nWs; ++i) {
    std::ostringstream os;
    os << "OutputWorkspace_" << i + 1;
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        os.str(), newWsName[i], Direction::Output));
    auto alg = createChildAlgorithm("RenameWorkspace");
    alg->setPropertyValue("InputWorkspace", inputWsName[i]);
    alg->setPropertyValue("OutputWorkspace", newWsName[i]);
    alg->setProperty("OverwriteExisting", overrideWorkspace);

    alg->executeAsChildAlg();

    Workspace_sptr renamedWs = alg->getProperty("OutputWorkspace");
    setProperty(os.str(), renamedWs);
  }
}

} // namespace Algorithms
} // namespace Mantid
