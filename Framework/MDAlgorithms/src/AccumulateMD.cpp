#include "MantidMDAlgorithms/AccumulateMD.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/HistoryView.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include <vector>
#include <Poco/File.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

/*
Return names of data sources which are found as a workspace or file.
Print to log whether data are found or not.
 */
std::vector<std::string>
filterToExistingSources(const std::vector<std::string> &input_data,
                        Kernel::Logger &g_log) {
  std::vector<std::string> existing_input_data;

  g_log.information() << "Finding data:" << std::endl;

  // If the entry exists as a workspace or a file then append it to existent
  // data list
  for (auto it = input_data.begin(); it != input_data.end(); ++it) {
    std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(*it);

    // Calls to the AnalysisDataService in algorithms like this should
    // ordinarily
    // be avoided, unfortunately we have little choice in this case.
    // If we gave FileFinder an absolute path it just returns it (whether or not
    // the file exists) so we must check the full path returned with
    // fileExists()
    if (AnalysisDataService::Instance().doesExist(*it) ||
        fileExists(filepath)) {
      existing_input_data.push_back(*it);
      g_log.information() << *it << " - FOUND" << std::endl;
    } else {
      g_log.information() << *it << " - NOT FOUND" << std::endl;
    }
  }

  return existing_input_data;
}

/*
Test if a file with this full path exists
*/
bool fileExists(const std::string &filename) {
  Poco::File test_file(filename);
  if (test_file.exists()) {
    return true;
  }
  return false;
}

/*
Return a vector of anything in input_data which is not in current_data
*/
std::vector<std::string> filterToNew(const std::vector<std::string> &input_data,
                                     const std::vector<std::string> &current_data) {
  std::vector<std::string> new_data;
  std::remove_copy_if(
      input_data.begin(), input_data.end(), std::back_inserter(new_data),
      [&current_data](const std::string &arg) {
        return (std::find(current_data.begin(), current_data.end(), arg) !=
                current_data.end());
      });

  return new_data;
}

/*
Return a vector of the names of files and workspaces which have previously added
to the workspace
*/
std::vector<std::string> getCurrentData(const WorkspaceHistory &ws_history) {
  std::vector<std::string> currentFilesAndWorkspaces;

  // TODO trawl history for workspace and file names
  // This is DataSources property of the original call of CreateMD and any
  // subsequent calls of AccumulateMD
  auto view = ws_history.createView();
  view->unrollAll();

  return currentFilesAndWorkspaces;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AccumulateMD)

/*
 Constructor
 */
AccumulateMD::AccumulateMD() {}

/*
 Destructor
 */
AccumulateMD::~AccumulateMD() {}

/// Algorithms name for identification. @see Algorithm::name
const std::string AccumulateMD::name() const { return "AccumulateMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int AccumulateMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AccumulateMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AccumulateMD::summary() const {
  return "Add new data to an existing MDHistoWorkspace";
}

/*
 Initialize the algorithm's properties.
 */
void AccumulateMD::init() {
  declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDHistoWorkspace to append data to.");

  declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "MDHistoWorkspace with new data appended.");

  declareProperty(
      new ArrayProperty<std::string>("DataSources", Direction::Input),
      "Input workspaces to process, or filenames to load and process");

  declareProperty(new ArrayProperty<double>("EFix", Direction::Input),
                  "datasource energy values in meV");

  declareProperty("Emode", "", Direction::Input);

  declareProperty(new ArrayProperty<double>("Alatt", Direction::Input),
                  "Lattice parameters");

  declareProperty(new ArrayProperty<double>("Angdeg", Direction::Input),
                  "Lattice angles");

  declareProperty(new ArrayProperty<double>("u", Direction::Input),
                  "Lattice vector parallel to neutron beam");

  declareProperty(
      new ArrayProperty<double>("v", Direction::Input),
      "Lattice vector perpendicular to neutron beam in the horizontal plane");

  declareProperty(new ArrayProperty<double>("Psi", Direction::Input),
                  "Psi rotation in degrees. Optional or one entry per run.");

  declareProperty(new ArrayProperty<double>("Gl", Direction::Input),
                  "gl rotation in degrees. Optional or one entry per run.");

  declareProperty(new ArrayProperty<double>("Gs", Direction::Input),
                  "gs rotation in degrees. Optional or one entry per run.");

  declareProperty(
      new PropertyWithValue<bool>("InPlace", false, Direction::Input),
      "Execute conversions to MD and Merge in one-step. Less "
      "memory overhead.");

  declareProperty(new PropertyWithValue<bool>("Clean", false, Direction::Input),
                  "Create workspace from fresh rather than appending to "
                  "existing workspace.");
}

/*
 Execute the algorithm.
 */
void AccumulateMD::exec() {

  const IMDHistoWorkspace_sptr input_ws = this->getProperty("InputWorkspace");
  std::vector<std::string> input_data = this->getProperty("DataSources");

  input_data = filterToExistingSources(input_data, g_log);

  // If we can't find any data, we can't do anything
  if (input_data.empty()) {
    g_log.warning() << "No data found matching input in " << this->name()
                    << std::endl;
    return; // POSSIBLE EXIT POINT
  }
  Algorithm::interruption_point();

  // If Clean=True then just call CreateMD to create a fresh workspace and
  // delete the old one, note this means we don't retain workspace history...
  bool do_clean = this->getProperty("Clean");
  if (do_clean) {
    Mantid::API::Algorithm_sptr createAlg = createChildAlgorithm("CreateMD");

    createAlg->setProperty("DataSources", input_data);
    createAlg->setPropertyValue("EFix", this->getProperty("EFix"));
    createAlg->setPropertyValue("EMode", this->getProperty("EMode"));
    createAlg->setPropertyValue("Alatt", this->getProperty("Alatt"));
    createAlg->setPropertyValue("Angdeg", this->getProperty("Angdeg"));
    createAlg->setPropertyValue("u", this->getProperty("u"));
    createAlg->setPropertyValue("v", this->getProperty("v"));
    createAlg->setPropertyValue("Psi", this->getProperty("Psi"));
    createAlg->setPropertyValue("Gl", this->getProperty("Gl"));
    createAlg->setPropertyValue("Gs", this->getProperty("Gs"));
    createAlg->setPropertyValue("OutputWorkspace",
                                this->getProperty("OutputWorkspace"));
    createAlg->setPropertyValue("InPlace", this->getProperty("InPlace"));
    createAlg->execute();

    return; // POSSIBLE EXIT POINT
  }
  Algorithm::interruption_point();

  // Trawl workspace history for filenames and workspace names
  const WorkspaceHistory wsHistory = input_ws->getHistory();
  std::vector<std::string> current_data = getCurrentData(wsHistory);

  // If there's no new data, we don't have anything to do
  input_data = filterToNew(input_data, current_data);
  if (input_data.empty()) {
    g_log.information() << "No new data to append to workspace in "
                        << this->name() << std::endl;
    return; // POSSIBLE EXIT POINT
  }
  Algorithm::interruption_point();

  // TODO if we reach here then new data exists to append to the input workspace
}

} // namespace MDAlgorithms
} // namespace Mantid
