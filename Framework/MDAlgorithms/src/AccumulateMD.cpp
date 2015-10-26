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
#include <boost/algorithm/string.hpp>

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
std::vector<std::string>
filterToNew(const std::vector<std::string> &input_data,
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
std::vector<std::string>
getHistoricalDataSources(const WorkspaceHistory &ws_history) {
  // Using a set so we only insert unique names
  std::set<std::string> historicalDataSources;

  // Get previously added data sources from DataSources property of the original
  // call of CreateMD and any subsequent calls of AccumulateMD
  auto view = ws_history.createView();
  view->unrollAll();
  const std::vector<HistoryItem> history_items = view->getAlgorithmsList();
  for (auto iter = history_items.begin(); iter != history_items.end(); ++iter) {
    auto alg_history = iter->getAlgorithmHistory();
    if (alg_history->name() == "CreateMD" ||
        alg_history->name() == "AccumulateMD") {
      auto props = alg_history->getProperties();
      for (auto propIter = props.begin(); propIter != props.end(); ++propIter) {
        PropertyHistory_const_sptr propHistory = *propIter;
        if (propHistory->name() == "DataSources") {
          insertDataSources(propHistory->value(), historicalDataSources);
        }
      }
    }
  }

  std::vector<std::string> result(historicalDataSources.begin(),
                                  historicalDataSources.end());
  return result;
}

/*
Split string of data sources from workspace history and insert them into
complete set of historical data sources
*/
void insertDataSources(const std::string &dataSources,
                       std::set<std::string> &historicalDataSources) {
  // Split the property string into a vector of data sources
  std::vector<std::string> data_split;
  boost::split(data_split, dataSources, boost::is_any_of(","));

  // Trim any whitespace from ends of each data source string
  std::for_each(
      data_split.begin(), data_split.end(),
      boost::bind(boost::algorithm::trim<std::string>, _1, std::locale()));

  // Insert each data source into our complete set of historical data sources
  for (auto it = data_split.begin(); it != data_split.end(); ++it) {
    historicalDataSources.insert(*it);
  }
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
  this->interruption_point();

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
  this->interruption_point();

  // Find what files and workspaces have already been included in the workspace.
  const WorkspaceHistory wsHistory = input_ws->getHistory();
  std::vector<std::string> current_data = getHistoricalDataSources(wsHistory);

  // If there's no new data, we don't have anything to do
  input_data = filterToNew(input_data, current_data);
  if (input_data.empty()) {
    g_log.information() << "No new data to append to workspace in "
                        << this->name() << std::endl;
    return; // POSSIBLE EXIT POINT
  }
  this->interruption_point();

  // If we reach here then new data exists to append to the input workspace
  this->progress(0.0);
  double progress_interval = 1.0 / input_data.size();
  int progress_count = 1;
  for (auto it = input_data.begin(); it != input_data.end();
       ++it, ++progress_count) {

    Workspace_sptr loaded_ws = getLoadedWs(*it);
    MDHistoWorkspace_sptr temp_md_ws =
        convertWorkspaceToMD(loaded_ws, this->getProperty("EMode"));
    setSampleParameters(); // goniometer, UB, efix etc
    appendWorkspace();

    // TODO delete temp_md_ws

    // Report progress to user and allow interrupt
    this->progress(progress_count * progress_interval);
    this->interruption_point();
  }
  this->progress(1.0); // Ensure progress is reported complete

  // TODO set outputworkspace?
  // TODO any cleanup to do?
}

/*
Convert the workspace to an MDWorkspace
*/
MDHistoWorkspace_sptr
AccumulateMD::convertWorkspaceToMD(Workspace_sptr loaded_ws,
                                   std::string emode) {
  // Find the Min Max extents
  Mantid::API::Algorithm_sptr minMaxAlg =
      createChildAlgorithm("ConvertToMDMinMaxGlobal");
  minMaxAlg->setProperty("InputWorkspace", loaded_ws);
  minMaxAlg->setProperty("QDimensions", "Q3D");
  minMaxAlg->setProperty("dEAnalysisMode", emode);
  minMaxAlg->executeAsChildAlg();

  // Convert to MD
  Mantid::API::Algorithm_sptr convertAlg = createChildAlgorithm("ConvertToMD");
  convertAlg->setProperty("InputWorkspace", loaded_ws);
  convertAlg->setProperty("QDimensions", "Q3D");
  convertAlg->setProperty("QConversionScales", "HKL");
  convertAlg->setProperty("dEAnalysisMode", emode);
  convertAlg->setProperty("MinValues",
                          minMaxAlg->getProperty("MinValues"));
  convertAlg->setProperty("MaxValues",
                          minMaxAlg->getPropertyValue("MaxValues"));
  convertAlg->setProperty("OverwriteExisting", true); // TODO check
  convertAlg->setPropertyValue("OutputWorkspace", "dummy");
  convertAlg->executeAsChildAlg();

  return convertAlg->getProperty("OutputWorkspace");
}

/*
Load data or get already loaded workspace
*/
Workspace_sptr AccumulateMD::getLoadedWs(std::string ws_name) {
  Workspace_sptr loaded_ws;

  if (!AnalysisDataService::Instance().doesExist(ws_name)) {
    Mantid::API::Algorithm_sptr loadAlg = createChildAlgorithm("Load");
    loadAlg->setProperty("Filename", ws_name);
    loadAlg->executeAsChildAlg();
    loaded_ws = loadAlg->getProperty("OutputWorkspace");
  } else {
    loaded_ws =
      AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(
        ws_name);
  }
  return loaded_ws;
}

} // namespace MDAlgorithms
} // namespace Mantid
