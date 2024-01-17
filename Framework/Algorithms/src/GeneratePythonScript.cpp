// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GeneratePythonScript.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ScriptBuilder.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/System.h"

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("GeneratePythonScript");
}

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GeneratePythonScript)

// Add alias as Export History
const std::string GeneratePythonScript::alias() const { return "ExportHistory"; }

/** Initialize the algorithm's properties.
 */
void GeneratePythonScript::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");

  std::vector<std::string> exts{".py"};

  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::OptionalSave, exts),
                  "The name of the file into which the workspace history will "
                  "be generated.");

  declareProperty("ScriptText", std::string(""), "Saves the history of the workspace to a variable.",
                  Direction::Output);

  declareProperty("UnrollAll", false, "Unroll all algorithms to show just their child algorithms.", Direction::Input);

  declareProperty("StartTimestamp", std::string(""), "The filter start time in the format YYYY-MM-DD HH:mm:ss",
                  Direction::Input);

  declareProperty("EndTimestamp", std::string(""), "The filter end time in the format YYYY-MM-DD HH:mm:ss",
                  Direction::Input);

  declareProperty("AppendTimestamp", false, "Appends the time the command was run as a comment afterwards",
                  Direction::Input);

  std::vector<std::string> saveVersions{"Specify Old", "Specify All", "Specify None"};
  declareProperty("SpecifyAlgorithmVersions", "Specify Old", std::make_shared<StringListValidator>(saveVersions),
                  "When to specify which algorithm version was used by Mantid.");

  declareProperty("IgnoreTheseAlgs", std::vector<std::string>(),
                  "A list of algorithms to filter out of the built script", Direction::Input);

  declareProperty("IgnoreTheseAlgProperties", std::vector<std::vector<std::string>>(),
                  "A list of algorithm properties to filter out of the built script", Direction::Input);
  declareProperty("AppendExecCount", false, "Whether execCount should be appended to the end of the built string");

  declareProperty("ExcludeHeader", false,
                  "Whether the header comments should be excluded from the beginning of the built string");
}

/** Execute the algorithm.
 */
void GeneratePythonScript::exec() {
  const Workspace_const_sptr ws = getProperty("InputWorkspace");
  const bool unrollAll = getProperty("UnrollAll");
  const std::string startTime = getProperty("StartTimestamp");
  const std::string endTime = getProperty("EndTimestamp");
  const std::string saveVersions = getProperty("SpecifyAlgorithmVersions");
  const bool appendTimestamp = getProperty("AppendTimestamp");
  const bool appendExecCount = getProperty("AppendExecCount");
  const bool excludeHeader = getProperty("ExcludeHeader");
  const std::vector<std::string> ignoreTheseAlgs = getProperty("IgnoreTheseAlgs");
  const std::vector<std::vector<std::string>> ignoreTheseAlgProperties = getProperty("IgnoreTheseAlgProperties");

  // Get the algorithm histories of the workspace.
  const WorkspaceHistory wsHistory = ws->getHistory();
  g_log.information() << "Number of history items: " << wsHistory.size() << '\n';

  auto view = wsHistory.createView();

  if (unrollAll) {
    view->unrollAll();
  }

  // Need at least a start time to do time filter
  if (!startTime.empty()) {
    if (endTime.empty()) {
      // If no end time was given then filter up to now
      view->filterBetweenExecDate(DateAndTime(startTime));
    } else {
      view->filterBetweenExecDate(DateAndTime(startTime), DateAndTime(endTime));
    }
  }

  std::string versionSpecificity;
  if (saveVersions == "Specify Old")
    versionSpecificity = "old";
  else if (saveVersions == "Specify None")
    versionSpecificity = "none";
  else
    versionSpecificity = "all";

  ScriptBuilder builder(view, versionSpecificity, appendTimestamp, ignoreTheseAlgs, ignoreTheseAlgProperties,
                        appendExecCount);
  std::string generatedScript;

  if (!excludeHeader) {

    const std::string mantidVersion = std::string(MantidVersion::version());
    const std::string gitSHA = std::string(MantidVersion::revisionFull());

    generatedScript += "# Python script generated by Mantid\n";
    generatedScript += "# Version " + mantidVersion + "\n";
    generatedScript += "# SHA-1 " + gitSHA + "\n\n";
  }

  generatedScript += builder.build();

  setPropertyValue("ScriptText", generatedScript);

  const std::string filename = getPropertyValue("Filename");

  if (!filename.empty()) {
    std::ofstream file(filename.c_str(), std::ofstream::trunc);
    file << generatedScript;
    file.flush();
    file.close();
  }
}

} // namespace Mantid::Algorithms
