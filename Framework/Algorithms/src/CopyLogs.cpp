// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyLogs.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Property.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CopyLogs)

using namespace API;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CopyLogs::name() const { return "CopyLogs"; }

/// Algorithm's version for identification. @see Algorithm::version
int CopyLogs::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CopyLogs::category() const { return "Utility\\Workspaces"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CopyLogs::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "Workspace to copy logs from.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::InOut),
                  "Workspace to copy logs to.");

  // options for the type of strategy to take
  std::vector<std::string> strategies{"WipeExisting", "MergeKeepExisting",
                                      "MergeReplaceExisting"};

  auto strategiesValidator =
      boost::make_shared<StringListValidator>(strategies);
  declareProperty("MergeStrategy", "MergeReplaceExisting", strategiesValidator,
                  "The type of merge strategy to use on the logs");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CopyLogs::exec() {
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");

  // get logs from input workspace
  Run &inputRun = inputWs->mutableRun();
  const auto &inputLogs = inputRun.getLogData();

  // get run from output workspace
  Run &outputRun = outputWs->mutableRun();

  std::string mode = getProperty("MergeStrategy");

  if (mode == "WipeExisting") {
    wipeExisting(inputLogs, outputRun);
  } else if (mode == "MergeKeepExisting") {
    mergeKeepExisting(inputLogs, outputRun);
  } else if (mode == "MergeReplaceExisting") {
    mergeReplaceExisting(inputLogs, outputRun);
  } else {
    throw std::runtime_error("Cannot copy logs using unknown merge strategy");
  }

  if (!outputWs->getName().empty())
    setProperty("OutputWorkspace", outputWs->getName());
}

/**
 * Copy logs from the input workspace to the output workspace
 * and replace any matching logs with the ones from the input workspace.
 */
void CopyLogs::mergeReplaceExisting(
    const std::vector<Kernel::Property *> &inputLogs, Run &outputRun) {
  for (auto prop : inputLogs) {
    // if the log exists, remove and replace it
    if (outputRun.hasProperty(prop->name())) {
      outputRun.removeLogData(prop->name());
    }
    outputRun.addLogData(prop->clone());
  }
}

/**
 * Copy logs from the input workspace to the output workspace
 * and don't replace any mathcing logs in the output workspace.
 */
void CopyLogs::mergeKeepExisting(
    const std::vector<Kernel::Property *> &inputLogs, Run &outputRun) {
  for (auto prop : inputLogs) {
    // add the log only if it doesn't already exist
    if (!outputRun.hasProperty(prop->name())) {
      outputRun.addLogData(prop->clone());
    }
  }
}

/**
 * Wipe any existing logs in the output workspace and replace
 * them with the logs from the input workspace.
 */
void CopyLogs::wipeExisting(const std::vector<Kernel::Property *> &inputLogs,
                            Run &outputRun) {
  auto outputLogs = outputRun.getLogData();

  // remove each of the logs from the second workspace
  for (auto &outputLog : outputLogs) {
    outputRun.removeLogData(outputLog->name());
  }

  // add all the logs from the new workspace
  for (auto inputLog : inputLogs) {
    outputRun.addLogData(inputLog->clone());
  }
}

} // namespace Algorithms
} // namespace Mantid
