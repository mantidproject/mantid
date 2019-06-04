// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CreateMD.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include <Poco/Path.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Box manager parameters for child algorithms
static const std::string SPLITINTO("2");
static const std::string SPLITTHRESHOLD("500");
static const std::string MAXRECURSIONDEPTH("20");

/*
 * Pad the vector of parameter values to the same size as data sources
 *
 * @param param_vector :: a vector of parameter values to pad
 * @param grow_to_size :: the parameter vector will be padded to this size
 */
void padParameterVector(std::vector<double> &param_vector,
                        const size_t grow_to_size) {
  if (param_vector.empty()) {
    param_vector.resize(grow_to_size, 0.0);
  } else if (param_vector.size() == 1) {
    param_vector.resize(grow_to_size, param_vector[0]);
  } else if (param_vector.size() != grow_to_size) {
    throw std::invalid_argument(
        "Psi, Gl, Gs and EFix must have one value per run.");
  }
}

/*
 * Returns true if any of the vectors in params are not empty
 *
 * @param params :: a vector of vectors of parameters
 * @returns true if any the vectors of parameters contain one or more values
 */
bool any_given(const std::vector<std::vector<double>> &params) {
  std::vector<double> param;
  for (const auto &iter : params) {
    param = iter;
    if (!param.empty()) {
      return true;
    }
  }
  return false;
}

/*
 * Returns true if all of the vectors in params are not empty
 *
 * @param params :: a vector of vectors of parameters
 * @returns true if all of the vectors of parameters contain one or more values
 */
bool all_given(const std::vector<std::vector<double>> &params) {
  std::vector<double> param;
  for (const auto &iter : params) {
    param = iter;
    if (param.empty()) {
      return false;
    }
  }
  return true;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateMD::name() const { return "CreateMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateMD::summary() const {
  return "Creates an MDWorkspace in the Q3D, HKL frame";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMD::init() {

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "MDEventWorkspace with new data appended.");

  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "DataSources",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>(),
          Direction::Input),
      "Input workspaces to process, or filenames to load and process");

  declareProperty(std::make_unique<ArrayProperty<double>>("EFix", Direction::Input),
                  "datasource energy values in meV");

  std::vector<std::string> e_mode_options{"Elastic", "Direct", "Indirect"};

  declareProperty("Emode", "Direct",
                  boost::make_shared<StringListValidator>(e_mode_options),
                  "Analysis mode ['Elastic', 'Direct', 'Indirect'].");

  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "Alatt",
          boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice parameters");

  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "Angdeg",
          boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice angles");

  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "u", boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice vector parallel to neutron beam");

  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "v", boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice vector perpendicular to neutron beam in the horizontal plane");

  declareProperty(std::make_unique<ArrayProperty<double>>("Psi", Direction::Input),
                  "Psi rotation in degrees. Optional or one entry per run.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Gl", Direction::Input),
                  "gl rotation in degrees. Optional or one entry per run.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Gs", Direction::Input),
                  "gs rotation in degrees. Optional or one entry per run.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("InPlace", true, Direction::Input),
      "Execute conversions to MD and Merge in one-step. Less "
      "memory overhead.");

  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalSave,
                                ".nxs"),
      "The name of the Nexus file to write, as a full or relative path.\n"
      "Only used if FileBackEnd is true.");
  setPropertySettings("Filename", std::make_unique<EnabledWhenProperty>(
                                      "FileBackEnd", IS_EQUAL_TO, "1"));

  declareProperty("FileBackEnd", false,
                  "If true, Filename must also be specified. The algorithm "
                  "will create the specified file in addition to an output "
                  "workspace. The workspace will load data from the file on "
                  "demand in order to reduce memory use.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMD::exec() {

  const std::string emode = this->getProperty("Emode");
  const std::vector<double> alatt = this->getProperty("Alatt");
  const std::vector<double> angdeg = this->getProperty("Angdeg");
  const std::vector<double> u = this->getProperty("u");
  const std::vector<double> v = this->getProperty("v");
  std::vector<double> psi = this->getProperty("Psi");
  std::vector<double> gl = this->getProperty("Gl");
  std::vector<double> gs = this->getProperty("Gs");
  std::vector<double> efix = this->getProperty("Efix");
  bool in_place = this->getProperty("InPlace");
  const std::vector<std::string> data_sources =
      this->getProperty("DataSources");
  const std::string out_filename = this->getProperty("Filename");
  const bool fileBackEnd = this->getProperty("FileBackEnd");

  const size_t entries = data_sources.size();

  padParameterVector(psi, entries);
  padParameterVector(gl, entries);
  padParameterVector(gs, entries);
  if (efix.empty()) {
    efix.push_back(-1.0);
  }
  padParameterVector(efix, entries);

  int counter = 0;
  std::vector<std::string> to_merge_names;
  std::string to_merge_name;
  MatrixWorkspace_sptr workspace;
  std::stringstream ws_name;
  IMDEventWorkspace_sptr run_md;
  Progress progress(this, 0.0, 1.0, entries + 1);
  for (unsigned long entry_number = 0; entry_number < entries;
       ++entry_number, ++counter) {
    ws_name.str(std::string());

    // If data source is not an existing workspace it must be a file we need to
    // load
    if (!AnalysisDataService::Instance().doesExist(
            data_sources[entry_number])) {
      // Strip off any file extension or path to leave just the stem (base)
      // filename
      std::string filename_noext =
          Poco::Path(data_sources[entry_number]).getBaseName();

      // Create workspace name of form {filename}_md_{n}
      ws_name << filename_noext << "_md_" << counter;
      to_merge_name = ws_name.str();
      workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          loadWs(data_sources[entry_number], to_merge_name));
    } else {
      workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve(data_sources[entry_number]));
      ws_name << data_sources[entry_number] << "_md";
      to_merge_name = ws_name.str();
    }

    // We cannot process in place until we have an output MDWorkspace to use.
    bool do_in_place = in_place && (counter > 0);
    run_md = single_run(workspace, emode, efix[entry_number], psi[entry_number],
                        gl[entry_number], gs[entry_number], do_in_place, alatt,
                        angdeg, u, v, out_filename, fileBackEnd, run_md);

    to_merge_names.push_back(to_merge_name);

    // We are stuck using ADS as we can't pass workspace pointers to MergeMD
    // There is currently no way to pass a list of workspace pointers
    if (!do_in_place) {
      AnalysisDataService::Instance().addOrReplace(to_merge_name, run_md);
    }

    progress.report();
  }

  Workspace_sptr output_workspace;
  if (to_merge_names.size() > 1 && !in_place) {
    progress.doReport("Merging loaded data into single workspace");
    output_workspace = merge_runs(to_merge_names);
  } else {
    output_workspace =
        AnalysisDataService::Instance().retrieve(to_merge_names[0]);
  }

  progress.report();

  // Clean up temporary workspaces
  for (const auto &name : to_merge_names) {
    AnalysisDataService::Instance().remove(name);
  }

  this->setProperty("OutputWorkspace", output_workspace);
}

/*
 * Load data from file
 *
 * @param filename :: the name of the file to load
 * @param wsname :: the name of the workspace to create with the loaded data in
 * @returns the workspace of loaded data
 */
Mantid::API::Workspace_sptr CreateMD::loadWs(const std::string &filename,
                                             const std::string &wsname) {
  Algorithm_sptr load_alg = createChildAlgorithm("Load");

  load_alg->setProperty("Filename", filename);
  load_alg->setPropertyValue("OutputWorkspace", wsname);
  load_alg->executeAsChildAlg();

  return load_alg->getProperty("OutputWorkspace");
}

/*
 * Set the sample log for the workspace
 *
 * @param workspace :: the workspace to add the log to
 * @param log_name :: the name of the log
 * @param log_number :: the value to record in the log
 */
void CreateMD::addSampleLog(Mantid::API::MatrixWorkspace_sptr workspace,
                            const std::string &log_name, double log_number) {
  Algorithm_sptr log_alg = createChildAlgorithm("AddSampleLog");

  log_alg->setProperty("Workspace", workspace);
  log_alg->setProperty("LogName", log_name);
  log_alg->setProperty("LogText", boost::lexical_cast<std::string>(log_number));
  log_alg->setProperty("LogType", "Number");
  // Force log to be of type double, even if integer value is passed
  log_alg->setProperty("NumberType", "Double");

  log_alg->executeAsChildAlg();
}

/*
 * Set the goniometer values for the workspace
 *
 * @param workspace :: the workspace to set the goniometer values in
 */
void CreateMD::setGoniometer(Mantid::API::MatrixWorkspace_sptr workspace) {
  Algorithm_sptr log_alg = createChildAlgorithm("SetGoniometer");
  if (!workspace->run().getProperty("gl")) {
    std::ostringstream temp_ss;
    temp_ss << "Value of gl in log is: "
            << workspace->run().getPropertyAsSingleValue("gl");
    throw std::invalid_argument(temp_ss.str());
  }
  log_alg->setProperty("Workspace", workspace);
  log_alg->setProperty("Axis0", "gl,0,0,1,1");
  log_alg->setProperty("Axis1", "gs,1,0,0,1");
  log_alg->setProperty("Axis2", "psi,0,1,0,1");

  log_alg->executeAsChildAlg();
}

/*
 * Set UB for the workspace
 *
 * @param workspace :: the workspace to set the UB matrix in
 * @param a :: length of crystal lattice parameter in angstroms
 * @param b :: length of crystal lattice parameter in angstroms
 * @param c :: length of crystal lattice parameter in angstroms
 * @param alpha :: lattice angle
 * @param beta :: lattice angle
 * @param gamma :: lattice angle
 * @param u :: lattice vector parallel to incident neutron beam
 * @param v :: lattice vector perpendicular to u in the horizontal plane
 */
void CreateMD::setUB(Mantid::API::MatrixWorkspace_sptr workspace, double a,
                     double b, double c, double alpha, double beta,
                     double gamma, const std::vector<double> &u,
                     const std::vector<double> &v) {
  Algorithm_sptr set_ub_alg = createChildAlgorithm("SetUB");

  set_ub_alg->setProperty("Workspace", workspace);
  set_ub_alg->setProperty("a", a);
  set_ub_alg->setProperty("b", b);
  set_ub_alg->setProperty("c", c);
  set_ub_alg->setProperty("alpha", alpha);
  set_ub_alg->setProperty("beta", beta);
  set_ub_alg->setProperty("gamma", gamma);
  set_ub_alg->setProperty("u", u);
  set_ub_alg->setProperty("v", v);
  set_ub_alg->executeAsChildAlg();
}

/*
 * Convert the workspace to an MDWorkspace
 *
 * @param workspace :: the workspace to convert to an MDWorkspace
 * @param analysis_mode :: the analysis mode "Direct", "Indirect" or "Elastic"
 * @param in_place :: true if merge step should be carried out at the same time
 * @out_mdws :: output workspace to use if merge step is carried out
 * @returns the output converted workspace
 */
Mantid::API::IMDEventWorkspace_sptr CreateMD::convertToMD(
    Mantid::API::Workspace_sptr workspace, const std::string &analysis_mode,
    bool in_place, const std::string &filebackend_filename,
    const bool filebackend, Mantid::API::IMDEventWorkspace_sptr out_mdws) {
  Algorithm_sptr min_max_alg = createChildAlgorithm("ConvertToMDMinMaxGlobal");
  min_max_alg->setProperty("InputWorkspace", workspace);
  min_max_alg->setProperty("QDimensions", "Q3D");
  min_max_alg->setProperty("dEAnalysisMode", analysis_mode);
  min_max_alg->executeAsChildAlg();
  std::string min_values = min_max_alg->getPropertyValue("MinValues");
  std::string max_values = min_max_alg->getPropertyValue("MaxValues");

  Algorithm_sptr convert_alg = createChildAlgorithm("ConvertToMD");
  convert_alg->setProperty("InputWorkspace", workspace);
  convert_alg->setProperty("QDimensions", "Q3D");
  convert_alg->setProperty("QConversionScales", "HKL");
  convert_alg->setProperty("dEAnalysisMode", analysis_mode);
  convert_alg->setPropertyValue("MinValues", min_values);
  convert_alg->setPropertyValue("MaxValues", max_values);
  // Use same box split settings in ConvertToMD and MergeMD
  // Otherwise InPlace=True or False will give different results
  convert_alg->setProperty("SplitInto", SPLITINTO);
  convert_alg->setProperty("SplitThreshold", SPLITTHRESHOLD);
  convert_alg->setProperty("MaxRecursionDepth", MAXRECURSIONDEPTH);
  convert_alg->setProperty("Filename", filebackend_filename);
  convert_alg->setProperty("FileBackEnd", filebackend);
  // OverwriteExisting=false means events are added to the existing workspace,
  // effectively doing the merge in place  (without using MergeMD)
  convert_alg->setProperty("OverwriteExisting", !in_place);
  if (in_place) {
    convert_alg->setProperty("OutputWorkspace", out_mdws);
  } else {
    convert_alg->setProperty("OutputWorkspace", "dummy");
  }
  convert_alg->executeAsChildAlg();

  return convert_alg->getProperty("OutputWorkspace");
}

/*
 * Merge input workspaces
 *
 * @param to_merge :: vector of workspaces to merge
 * @returns MDEventWorkspace containing merged data of input workspaces
 */
Mantid::API::IMDEventWorkspace_sptr
CreateMD::merge_runs(const std::vector<std::string> &to_merge) {
  Algorithm_sptr merge_alg = createChildAlgorithm("MergeMD");

  merge_alg->setProperty("InputWorkspaces", to_merge);
  merge_alg->setPropertyValue("OutputWorkspace", "dummy");
  // Use same box split settings in ConvertToMD and MergeMD
  // Otherwise InPlace=True or False will give different results
  merge_alg->setProperty("SplitInto", SPLITINTO);
  merge_alg->setProperty("SplitThreshold", SPLITTHRESHOLD);
  merge_alg->setProperty("MaxRecursionDepth", MAXRECURSIONDEPTH);
  merge_alg->executeAsChildAlg();

  return merge_alg->getProperty("OutputWorkspace");
}

/*
 * Add parameter logs and convert to MD for a single run
 *
 * @param input_workspace :: datasource workspace
 * @param emode :: analysis mode "Elastic", "Direct" or "Indirect"
 * @param efix :: datasource energy values in meV
 * @param psi :: goniometer rotation in degrees
 * @param gl :: goniometer rotation in degrees
 * @param gs :: goniometer rotation in degrees
 * @param in_place :: do merge step at the same time as converting to
 *MDWorkspace
 * @param alatt :: length of crystal lattice parameter in angstroms
 * @param angdeg :: lattice angle
 * @param u :: lattice vector parallel to incident neutron beam
 * @param v :: lattice vector perpendicular to u in the horizontal plane
 * @param out_mdws :output workspace to use if merge step is carried out
 */
Mantid::API::IMDEventWorkspace_sptr CreateMD::single_run(
    Mantid::API::MatrixWorkspace_sptr input_workspace, const std::string &emode,
    double efix, double psi, double gl, double gs, bool in_place,
    const std::vector<double> &alatt, const std::vector<double> &angdeg,
    const std::vector<double> &u, const std::vector<double> &v,
    const std::string &filebackend_filename, const bool filebackend,
    Mantid::API::IMDEventWorkspace_sptr out_mdws) {

  std::vector<std::vector<double>> ub_params{alatt, angdeg, u, v};

  if (any_given(ub_params) && !all_given(ub_params)) {
    throw std::invalid_argument(
        "Either specify all of alatt, angledeg, u, v or none of them");
  } else {
    if (input_workspace->sample().hasOrientedLattice()) {
      g_log.warning() << "Sample already has a UB. This will not be "
                         "overwritten. Use ClearUB and re-run.\n";
    } else {
      setUB(input_workspace, alatt[0], alatt[1], alatt[2], angdeg[0], angdeg[1],
            angdeg[2], u, v);
    }

    if (efix > 0.0) {
      addSampleLog(input_workspace, "Ei", efix);
    }

    addSampleLog(input_workspace, "gl", gl);
    addSampleLog(input_workspace, "gs", gs);
    addSampleLog(input_workspace, "psi", psi);
    setGoniometer(input_workspace);

    return convertToMD(input_workspace, emode, in_place, filebackend_filename,
                       filebackend, out_mdws);
  }
}

/*
 * Validate input properties
 *
 * @returns map with keys corresponding to properties with errors and values
 *containing the error messages
 */
std::map<std::string, std::string> CreateMD::validateInputs() {
  // Create the map
  std::map<std::string, std::string> validation_output;

  // Get properties to validate
  const std::vector<std::string> data_sources =
      this->getProperty("DataSources");
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
    validation_output["Filename"] =
        "Filename must be given if FileBackEnd is required.";
  }

  const size_t ws_entries = data_sources.size();
  for (const auto &source : data_sources) {
    if (!dataExists(source)) {
      validation_output["DataSources"] =
          "All given data sources must exist. "
          "For files, ensure the path is added to "
          "Mantid's 'Data Search Directories'";
    }
  }

  if (u.size() != 3) {
    validation_output["u"] = "u must have 3 components";
  }
  if (v.size() != 3) {
    validation_output["v"] = "v must have 3 components";
  }
  if (alatt.size() != 3) {
    validation_output["Alatt"] = "Lattice parameters must have 3 components";
  }
  if (angdeg.size() != 3) {
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
    validation_output["EFix"] =
        "Either specify a single EFix value, or as many "
        "as there are input datasources";
  }

  return validation_output;
}

} // namespace MDAlgorithms
} // namespace Mantid
