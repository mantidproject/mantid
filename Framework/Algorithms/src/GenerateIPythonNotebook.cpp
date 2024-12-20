// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GenerateIPythonNotebook.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NotebookBuilder.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ListValidator.h"

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("GenerateIPythonNotebook");
}

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateIPythonNotebook)

/** Initialize the algorithm's properties.
 */
void GenerateIPythonNotebook::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");

  std::vector<std::string> exts{".ipynb"};

  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::OptionalSave, exts),
                  "The name of the file into which the workspace history will "
                  "be generated.");
  declareProperty("NotebookText", std::string(""), "Saves the history of the workspace to a variable.",
                  Direction::Output);
  getPointerToProperty("NotebookText")->setAutoTrim(false);

  declareProperty("UnrollAll", false, "Unroll all algorithms to show just their child algorithms.", Direction::Input);

  declareProperty("StartTimestamp", std::string(""), "The filter start time in the format YYYY-MM-DD HH:mm:ss",
                  Direction::Input);
  declareProperty("EndTimestamp", std::string(""), "The filter end time in the format YYYY-MM-DD HH:mm:ss",
                  Direction::Input);

  std::vector<std::string> saveVersions{"Specify Old", "Specify All", "Specify None"};
  declareProperty("SpecifyAlgorithmVersions", "Specify Old", std::make_shared<StringListValidator>(saveVersions),
                  "When to specify which algorithm version was used by Mantid.");
}

/** Execute the algorithm.
 */
void GenerateIPythonNotebook::exec() {
  const Workspace_const_sptr ws = getProperty("InputWorkspace");
  const bool unrollAll = getProperty("UnrollAll");
  const std::string startTime = getProperty("StartTimestamp");
  const std::string endTime = getProperty("EndTimestamp");
  const std::string saveVersions = getProperty("SpecifyAlgorithmVersions");

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

  NotebookBuilder builder(view, versionSpecificity);
  std::string generatedNotebook;
  generatedNotebook += builder.build(ws->getName(), ws->getTitle(), ws->getComment());

  setPropertyValue("NotebookText", generatedNotebook);

  const std::string filename = getPropertyValue("Filename");

  if (!filename.empty()) {
    std::ofstream file(filename.c_str(), std::ofstream::trunc);
    file << generatedNotebook;
    file.flush();
    file.close();
  }
}

} // namespace Mantid::Algorithms
