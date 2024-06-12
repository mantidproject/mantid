// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/AccumulateMD.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/HistoryView.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <Poco/File.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::MDAlgorithms {

/*
 * Reduce the vector of input data to only data files and workspaces which can
 * be found
 * @param input_data :: Vector of input data sources
 * @param psi :: Vector of goniometer angle psi containing a value for each data
 * source
 * @param gl :: Vector of goniometer anglegl containing a value for each data
 * source
 * @param gs :: Vector of goniometer angle gs containing a value for each data
 * source
 * @param efix :: Vector of data source energy values in meV
 * @returns names of data sources which cannot be found
 */
std::string filterToExistingSources(std::vector<std::string> &input_data, std::vector<double> &psi,
                                    std::vector<double> &gl, std::vector<double> &gs, std::vector<double> &efix) {
  std::ostringstream nonexistent;
  for (size_t i = input_data.size(); i > 0; i--) {
    if (!dataExists(input_data[i - 1])) {
      nonexistent << input_data[i - 1] << ",";
      input_data.erase(input_data.begin() + i - 1);
      psi.erase(psi.begin() + i - 1);
      gl.erase(gl.begin() + i - 1);
      gs.erase(gs.begin() + i - 1);
      efix.erase(efix.begin() + i - 1);
    }
  }
  return nonexistent.str();
}

/*
 * Return true if dataName is an existing workspace or file
 * @param data_name :: Workspace name or file name
 * @returns true if a workspace or file with given name exists
 */
bool dataExists(const std::string &data_name) {
  const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(data_name);
  // Calls to the ADS in algorithms like this should ordinarily
  // be avoided, unfortunately we have little choice in this case.
  // If we gave FileFinder an absolute path it just returns it (whether or not
  // the file exists) so we must also check the full path returned with
  // fileExists()
  return (AnalysisDataService::Instance().doesExist(data_name) || fileExists(filepath));
}

/*
 * Test if a file with this full path exists
 * @param filename :: full path of a file to test existence of
 * @returns true if the file exists
 */
bool fileExists(const std::string &filename) {
  if (filename.empty())
    return false;
  Poco::File test_file(filename);
  return test_file.exists();
}

/*
 * Remove anything from input_data which is already in current_data
 * @param input_data :: Vector of input data sources
 * @param current_data :: Vector of data sources previously appended to
 * workspace
 * @param psi :: Vector of goniometer angle psi containing a value for each data
 * source
 * @param gl :: Vector of goniometer anglegl containing a value for each data
 * source
 * @param gs :: Vector of goniometer angle gs containing a value for each data
 * source
 * @param efix :: Vector of data source energy values in meV
 * @returns data sources which are already in the workspace
 */
std::string filterToNew(std::vector<std::string> &input_data, const std::vector<std::string> &current_data,
                        std::vector<double> &psi, std::vector<double> &gl, std::vector<double> &gs,
                        std::vector<double> &efix) {
  std::ostringstream old_sources;
  for (size_t i = input_data.size(); i > 0; i--) {
    if (appearsInCurrentData(input_data[i - 1], current_data)) {
      old_sources << input_data[i - 1] << ",";
      input_data.erase(input_data.begin() + i - 1);
      psi.erase(psi.begin() + i - 1);
      gl.erase(gl.begin() + i - 1);
      gs.erase(gs.begin() + i - 1);
      efix.erase(efix.begin() + i - 1);
    }
  }
  return old_sources.str();
}

/*
 * Check if the named data source is in the vector of data currently in the
 * workspace
 * @param data_source :: Name of a data source
 * @param current_data :: Vector of data sources previously appended to
 * workspace
 * @returns true if the named data source appears in the vector of current data
 */
bool appearsInCurrentData(const std::string &data_source, const std::vector<std::string> &current_data) {
  return std::any_of(current_data.rbegin(), current_data.rend(),
                     [&data_source](const auto &source) { return data_source == source; });
}

/*
 * Return a vector of the names of files and workspaces which have been
 * previously added to the workspace
 * @param ws_history :: History of the workspace
 * @returns a vector of the names of data_sources which have previously been
 * appended to the workspace
 */
std::vector<std::string> getHistoricalDataSources(const WorkspaceHistory &ws_history,
                                                  const std::string &create_alg_name,
                                                  const std::string &accumulate_alg_name) {
  // Using a set so we only insert unique names
  std::unordered_set<std::string> historical_data_sources;

  // Get previously added data sources from DataSources property of the original
  // call of CreateMD and any subsequent calls of AccumulateMD
  auto view = ws_history.createView();
  view->unrollAll();
  const std::vector<HistoryItem> history_items = view->getAlgorithmsList();
  for (const auto &history_item : history_items) {
    auto alg_history = history_item.getAlgorithmHistory();
    if (alg_history->name() == create_alg_name || alg_history->name() == accumulate_alg_name) {
      auto props = alg_history->getProperties();
      for (const auto &prop : props) {
        PropertyHistory_const_sptr prop_history = prop;
        if (prop_history->name() == "DataSources") {
          insertDataSources(prop_history->value(), historical_data_sources);
        }
      }
    }
  }

  std::vector<std::string> result(historical_data_sources.begin(), historical_data_sources.end());
  return result;
}

/*
 * Split string of data sources from workspace history and insert them into
 * complete set of historical data sources
 * @param data_sources :: string from workspace history containing list of data
 * sources
 * @param historical_data_sources :: set of data sources
 */
void insertDataSources(const std::string &data_sources, std::unordered_set<std::string> &historical_data_sources) {
  // Split the property string into a vector of data sources
  std::vector<std::string> data_split;
  boost::split(data_split, data_sources, boost::is_any_of(","));

  // Trim any whitespace from ends of each data source string
  std::for_each(data_split.begin(), data_split.end(),
                std::bind(boost::algorithm::trim<std::string>, std::placeholders::_1, std::locale()));

  // Insert each data source into our complete set of historical data sources
  historical_data_sources.insert(data_split.begin(), data_split.end());
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AccumulateMD)

/// Algorithms name for identification. @see Algorithm::name
const std::string AccumulateMD::name() const { return "AccumulateMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int AccumulateMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AccumulateMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AccumulateMD::summary() const { return "Add new data to an existing MDHistoWorkspace"; }

/*
 * Initialize the algorithm's properties.
 */
void AccumulateMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace to append data to.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "MDEventWorkspace with new data appended.");

  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "DataSources", std::make_shared<MandatoryValidator<std::vector<std::string>>>(), Direction::Input),
      "Input workspaces to process, or filenames to load and process");

  declareProperty(std::make_unique<ArrayProperty<double>>("EFix", Direction::Input), "datasource energy values in meV");

  std::vector<std::string> e_mode_options{"Elastic", "Direct", "Indirect"};

  declareProperty("Emode", "Direct", std::make_shared<StringListValidator>(e_mode_options),
                  "Analysis mode ['Elastic', 'Direct', 'Indirect'].");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Alatt", std::make_shared<MandatoryValidator<std::vector<double>>>(), Direction::Input),
                  "Lattice parameters");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Angdeg", std::make_shared<MandatoryValidator<std::vector<double>>>(), Direction::Input),
                  "Lattice angles");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "u", std::make_shared<MandatoryValidator<std::vector<double>>>(), Direction::Input),
                  "Lattice vector parallel to neutron beam");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "v", std::make_shared<MandatoryValidator<std::vector<double>>>(), Direction::Input),
                  "Lattice vector perpendicular to neutron beam in the horizontal plane");

  declareProperty(std::make_unique<ArrayProperty<double>>("Psi", Direction::Input),
                  "Psi rotation in degrees. Optional or one entry per run.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Gl", Direction::Input),
                  "gl rotation in degrees. Optional or one entry per run.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Gs", Direction::Input),
                  "gs rotation in degrees. Optional or one entry per run.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("InPlace", true, Direction::Input),
                  "Execute conversions to MD and Merge in one-step. Less "
                  "memory overhead.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("Clean", false, Direction::Input),
                  "Create workspace from fresh rather than appending to "
                  "existing workspace data.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalSave, ".nxs"),
                  "The name of the Nexus file to write, as a full or relative path.\n"
                  "Only used if FileBackEnd is true.");
  setPropertySettings("Filename", std::make_unique<EnabledWhenProperty>("FileBackEnd", IS_EQUAL_TO, "1"));

  declareProperty("FileBackEnd", false,
                  "If true, Filename must also be specified. The algorithm "
                  "will create the specified file in addition to an output "
                  "workspace. The workspace will load data from the file on "
                  "demand in order to reduce memory use.");
}

/*
 * Execute the algorithm.
 */
void AccumulateMD::exec() {

  IMDEventWorkspace_sptr input_ws = this->getProperty("InputWorkspace");
  std::vector<std::string> input_data = this->getProperty("DataSources");

  const std::string out_filename = this->getProperty("Filename");
  const bool filebackend = this->getProperty("FileBackEnd");

  std::vector<double> psi = this->getProperty("Psi");
  padParameterVector(psi, input_data.size());
  std::vector<double> gl = this->getProperty("Gl");
  padParameterVector(gl, input_data.size());
  std::vector<double> gs = this->getProperty("Gs");
  padParameterVector(gs, input_data.size());
  std::vector<double> efix = this->getProperty("EFix");
  padParameterVector(efix, input_data.size());

  // Create progress reporting object
  // Progress prog = Progress(this, 0.0, 1.0, 2);
  this->progress(0.0);

  const std::string nonexistent = filterToExistingSources(input_data, psi, gl, gs, efix);
  g_log.notice() << "These data sources were not found: " << nonexistent << '\n';

  // If we can't find any data, we can't do anything
  if (input_data.empty()) {
    g_log.warning() << "No data found matching input in " << this->name() << '\n';
    this->setProperty("OutputWorkspace", input_ws);
    return; // POSSIBLE EXIT POINT
  }
  this->interruption_point();

  // If Clean=True then just call CreateMD to create a fresh workspace and
  // delete the old one, note this means we don't retain workspace history...
  bool do_clean = this->getProperty("Clean");
  if (do_clean) {
    this->progress(0.5);
    IMDEventWorkspace_sptr out_ws = createMDWorkspace(input_data, psi, gl, gs, efix, out_filename, filebackend);
    this->setProperty("OutputWorkspace", out_ws);
    g_log.notice() << this->name() << " successfully created a clean workspace\n";
    this->progress(1.0);
    return; // POSSIBLE EXIT POINT
  }
  this->interruption_point();

  // Find what files and workspaces have already been included in the workspace.
  const WorkspaceHistory ws_history = input_ws->getHistory();
  // Get name from algorithm like this so that an error is thrown if the
  // name of the algorithm is changed
  Algorithm_sptr create_alg = createChildAlgorithm("CreateMD");
  const std::vector<std::string> current_data = getHistoricalDataSources(ws_history, create_alg->name(), this->name());

  // If there's no new data, we don't have anything to do
  const std::string old_sources = filterToNew(input_data, current_data, psi, gl, gs, efix);
  g_log.notice() << "Data from these sources are already in the workspace: " << old_sources << '\n';

  if (input_data.empty()) {
    g_log.notice() << "No new data to append to workspace in " << this->name() << '\n';
    this->setProperty("OutputWorkspace", input_ws);
    return; // POSSIBLE EXIT POINT
  }
  this->interruption_point();

  // If we reach here then new data exists to append to the input workspace
  // Use CreateMD with the new data to make a temp workspace
  // Merge the temp workspace with the input workspace using MergeMD
  IMDEventWorkspace_sptr tmp_ws = createMDWorkspace(input_data, psi, gl, gs, efix, "", false);
  this->interruption_point();
  this->progress(0.5); // Report as CreateMD is complete

  const std::string temp_ws_name = "TEMP_WORKSPACE_ACCUMULATEMD";
  // Currently have to use ADS here as list of workspaces can only be passed as
  // a list of workspace names as a string
  AnalysisDataService::Instance().add(temp_ws_name, tmp_ws);
  std::string ws_names_to_merge = input_ws->getName();
  ws_names_to_merge.append(",");
  ws_names_to_merge.append(temp_ws_name);

  Algorithm_sptr merge_alg = createChildAlgorithm("MergeMD");
  merge_alg->setProperty("InputWorkspaces", ws_names_to_merge);
  merge_alg->executeAsChildAlg();

  API::IMDEventWorkspace_sptr out_ws = merge_alg->getProperty("OutputWorkspace");

  this->setProperty("OutputWorkspace", out_ws);
  g_log.notice() << this->name() << " successfully appended data\n";

  this->progress(1.0); // Report as MergeMD is complete

  // Clean up temporary workspace
  AnalysisDataService::Instance().remove(temp_ws_name);
}

/*
 * Use the CreateMD algorithm to create an MD workspace
 * @param data_sources :: Vector of input data sources
 * @param psi :: Vector of goniometer angle psi containing a value for each data
 * source
 * @param gl :: Vector of goniometer anglegl containing a value for each data
 * source
 * @param gs :: Vector of goniometer angle gs containing a value for each data
 * source
 * @param efix :: Vector of data source energy values in meV
 * @returns the newly created workspace
 */
IMDEventWorkspace_sptr AccumulateMD::createMDWorkspace(const std::vector<std::string> &data_sources,
                                                       const std::vector<double> &psi, const std::vector<double> &gl,
                                                       const std::vector<double> &gs, const std::vector<double> &efix,
                                                       const std::string &filename, const bool filebackend) {

  Algorithm_sptr create_alg = createChildAlgorithm("CreateMD");

  create_alg->setProperty("DataSources", data_sources);
  create_alg->setProperty("EFix", efix);
  create_alg->setPropertyValue("EMode", this->getPropertyValue("EMode"));
  create_alg->setPropertyValue("Alatt", this->getPropertyValue("Alatt"));
  create_alg->setPropertyValue("Angdeg", this->getPropertyValue("Angdeg"));
  create_alg->setPropertyValue("u", this->getPropertyValue("u"));
  create_alg->setPropertyValue("v", this->getPropertyValue("v"));
  create_alg->setProperty("Psi", psi);
  create_alg->setProperty("Gl", gl);
  create_alg->setProperty("Gs", gs);
  create_alg->setPropertyValue("InPlace", this->getPropertyValue("InPlace"));
  if (filebackend) {
    create_alg->setProperty("Filename", filename);
    create_alg->setProperty("FileBackEnd", filebackend);
  }
  create_alg->executeAsChildAlg();

  return create_alg->getProperty("OutputWorkspace");
}

/*
 * Validate the input properties
 * @returns a map of properties names with errors
 */
std::map<std::string, std::string> AccumulateMD::validateInputs() {
  // Create the map
  std::map<std::string, std::string> validation_output;

  // Get properties to validate
  const std::vector<std::string> data_sources = this->getProperty("DataSources");
  const std::vector<double> u = this->getProperty("u");
  const std::vector<double> v = this->getProperty("v");
  const std::vector<double> alatt = this->getProperty("Alatt");
  const std::vector<double> angdeg = this->getProperty("Angdeg");
  const std::vector<double> psi = this->getProperty("Psi");
  const std::vector<double> gl = this->getProperty("Gl");
  const std::vector<double> gs = this->getProperty("Gs");
  const std::vector<double> efix = this->getProperty("Efix");
  const std::string filename = this->getProperty("Filename");
  const bool fileBackEnd = this->getProperty("FileBackEnd");

  if (fileBackEnd && filename.empty()) {
    validation_output["Filename"] = "Filename must be given if FileBackEnd is required.";
  }

  const size_t ws_entries = data_sources.size();

  if (u.size() < 3) {
    validation_output["u"] = "u must have 3 components";
  }
  if (v.size() < 3) {
    validation_output["v"] = "v must have 3 components";
  }
  if (alatt.size() < 3) {
    validation_output["Alatt"] = "Lattice parameters must have 3 components";
  }
  if (angdeg.size() < 3) {
    validation_output["Angdeg"] = "Angle must have 3 components";
  }
  if (!psi.empty() && psi.size() != ws_entries) {
    validation_output["Psi"] = "If Psi is given an entry "
                               "should be provided for "
                               "every input datasource";
  }
  if (!gl.empty() && gl.size() != ws_entries) {
    validation_output["Gl"] = "If Gl is given an entry "
                              "should be provided for "
                              "every input datasource";
  }
  if (!gs.empty() && gs.size() != ws_entries) {
    validation_output["Gs"] = "If Gs is given an entry "
                              "should be provided for "
                              "every input datasource";
  }
  if (efix.size() > 1 && efix.size() != ws_entries) {
    validation_output["EFix"] = "Either specify a single EFix value, or as many "
                                "as there are input datasources";
  }

  return validation_output;
}

} // namespace Mantid::MDAlgorithms
