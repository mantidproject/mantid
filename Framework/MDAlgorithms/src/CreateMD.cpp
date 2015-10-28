#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/CreateMD.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include <boost/filesystem.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

/*
 * Pad the vector of parameter values to the same size as data sources
 */
void padParameterVector(std::vector<double> &param_vector,
                        unsigned long grow_to_size) {
  if (param_vector.size() == 0) {
    param_vector.resize(grow_to_size, 0.0);
  } else if (param_vector.size() == 1) {
    param_vector.resize(grow_to_size, param_vector[0]);
  }
}

/*
 * Returns true if any of the vectors in params are not empty
 */
bool any_given(const std::vector<std::vector<double>> &params) {
  std::vector<double> param;
  for (auto iter = params.begin(); iter != params.end(); ++iter) {
    param = *iter;
    if (!param.empty()) {
      return true;
    }
  }
  return false;
}

/*
 * Returns true if all of the vectors in params are not empty
 */
bool all_given(const std::vector<std::vector<double>> &params) {
  std::vector<double> param;
  for (auto iter = params.begin(); iter != params.end(); ++iter) {
    param = *iter;
    if (param.empty()) {
      return false;
    }
  }
  return true;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreateMD::CreateMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreateMD::~CreateMD() {}

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

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "MDHistoWorkspace with new data appended.");

  declareProperty(
      new ArrayProperty<std::string>(
          "DataSources",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>(),
          Direction::Input),
      "Input workspaces to process, or filenames to load and process");

  declareProperty(new ArrayProperty<double>("EFix", Direction::Input),
                  "datasource energy values in meV");

  std::vector<std::string> e_mode_options;
  e_mode_options.push_back("Elastic");
  e_mode_options.push_back("Direct");
  e_mode_options.push_back("Indirect");

  declareProperty("Emode", "Direct",
                  boost::make_shared<StringListValidator>(e_mode_options),
                  "Analysis mode ['Elastic', 'Direct', 'Indirect'].");

  declareProperty(
      new ArrayProperty<double>(
          "Alatt",
          boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice parameters");

  declareProperty(
      new ArrayProperty<double>(
          "Angdeg",
          boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice angles");

  declareProperty(
      new ArrayProperty<double>(
          "u", boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
      "Lattice vector parallel to neutron beam");

  declareProperty(
      new ArrayProperty<double>(
          "v", boost::make_shared<MandatoryValidator<std::vector<double>>>(),
          Direction::Input),
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

  const unsigned long entries = data_sources.size();

  padParameterVector(psi, entries);
  padParameterVector(gl, entries);
  padParameterVector(gs, entries);
  padParameterVector(efix, entries);

  int counter = 0;
  std::vector<std::string> to_merge_names;
  std::string to_merge_name;
  Workspace_sptr workspace;
  std::stringstream ws_name;
  for (unsigned long entry_number = 0; entry_number < entries; ++entry_number) {
    ws_name.str(std::string());

    // If data source is not an existing workspace it must be a file we need to
    // load
    if (!AnalysisDataService::Instance().doesExist(
            data_sources[entry_number])) {
      // Strip off any file extension or path to leave just the stem filename
      std::string filename_noext =
          boost::filesystem::path(data_sources[entry_number]).stem().string();

      // Create workspace name of form {filename}_md_{n}
      ws_name << filename_noext << "_md_" << counter;
      to_merge_name = ws_name.str();
      workspace = loadWs(data_sources[entry_number], to_merge_name);
    } else {
      workspace =
          AnalysisDataService::Instance().retrieve(data_sources[entry_number]);
      ws_name << data_sources[entry_number] << "_md";
      to_merge_name = ws_name.str();
    }

    // We cannot process in place until we have an output MDWorkspace to use.
    bool do_in_place = in_place && (counter > 0);
    Workspace_sptr run_md =
        single_run(workspace, emode, efix[entry_number], psi[entry_number],
                   gl[entry_number], gs[entry_number], do_in_place, alatt,
                   angdeg, u, v, run_md);
    to_merge_names.push_back(to_merge_name);

    AnalysisDataService::Instance().addOrReplace(to_merge_name, run_md);

    counter++;
  }

  Workspace_sptr output_workspace;
  if (to_merge_names.size() > 1 && !in_place) {
    output_workspace = merge_runs(to_merge_names);
  } else {
    output_workspace =
        AnalysisDataService::Instance().retrieve(to_merge_names[0]);
  }

  // Clean up temporary workspaces
  for (auto ws_iter = to_merge_names.begin(); ws_iter != to_merge_names.end();
       ++ws_iter) {
    AnalysisDataService::Instance().remove(*ws_iter);
  }

  this->setProperty("OutputWorkspace", output_workspace);
}

/*
 * Load data from file
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
 */
void CreateMD::addSampleLog(Mantid::API::Workspace_sptr workspace,
                            const std::string &log_name, double log_number) {
  Algorithm_sptr log_alg = createChildAlgorithm("AddSampleLog");

  std::stringstream log_num_str;
  log_num_str << log_number;

  log_alg->setProperty("Workspace", workspace);
  log_alg->setProperty("LogName", log_name);
  log_alg->setProperty("LogText", log_num_str.str());
  log_alg->setProperty("LogType", "Number");

  log_alg->executeAsChildAlg();
}

/*
 * Set the goniometer values for the workspace
 */
void CreateMD::setGoniometer(Mantid::API::Workspace_sptr workspace) {
  Algorithm_sptr log_alg = createChildAlgorithm("SetGoniometer");

  log_alg->setProperty("Workspace", workspace);
  log_alg->setProperty("Axis0", "gl,0,0,1,1");
  log_alg->setProperty("Axis1", "gs,1,0,0,1");
  log_alg->setProperty("Axis2", "psi,0,1,0,1");

  log_alg->executeAsChildAlg();
}

/*
 * Set UB for the workspace
 */
void CreateMD::setUB(Mantid::API::Workspace_sptr workspace, double a, double b,
                     double c, double alpha, double beta, double gamma,
                     const std::vector<double> &u,
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
 */
Mantid::API::Workspace_sptr
CreateMD::convertToMD(Mantid::API::Workspace_sptr workspace,
                      const std::string &analysis_mode, bool in_place,
                      Mantid::API::Workspace_sptr out_mdws) {
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
 */
Mantid::API::Workspace_sptr
CreateMD::merge_runs(const std::vector<std::string> &to_merge) {
  Algorithm_sptr merge_alg = createChildAlgorithm("MergeMD");

  merge_alg->setProperty("InputWorkspaces", to_merge);
  merge_alg->setPropertyValue("OutputWorkspace", "dummy");
  merge_alg->executeAsChildAlg();

  return merge_alg->getProperty("OutputWorkspace");
}

/*
 * Add parameter logs and convert to MD for a single run
 */
Mantid::API::Workspace_sptr CreateMD::single_run(
    Mantid::API::Workspace_sptr input_workspace, const std::string &emode,
    double efix, double psi, double gl, double gs, bool in_place,
    const std::vector<double> &alatt, const std::vector<double> &angdeg,
    const std::vector<double> &u, const std::vector<double> &v,
    Mantid::API::Workspace_sptr out_mdws) {

  std::vector<std::vector<double>> ub_params;
  ub_params.push_back(alatt);
  ub_params.push_back(angdeg);
  ub_params.push_back(u);
  ub_params.push_back(v);

  std::vector<double> goniometer_params;
  goniometer_params.push_back(psi);
  goniometer_params.push_back(gl);
  goniometer_params.push_back(gs);

  if (any_given(ub_params) && !all_given(ub_params)) {
    throw std::invalid_argument(
        "Either specify all of alatt, angledeg, u, v or none of them");
  } else {
    if (false) { // TODO check if UB set already
      g_log.warning() << "Sample already has a UB. This will not be "
                         "overwritten. Use ClearUB and re-run." << std::endl;
    } else {
      setUB(input_workspace, alatt[0], alatt[1], alatt[2], angdeg[0], angdeg[1],
            angdeg[2], u, v);
    }

    if (efix > 0.0) {
      addSampleLog(input_workspace, "Ei", efix);
    }

    if (std::any_of(goniometer_params.begin(), goniometer_params.end(),
                    [](const std::string &param) { return param.empty(); })) {
      addSampleLog(input_workspace, "gl", gl);
      addSampleLog(input_workspace, "gs", gs);
      addSampleLog(input_workspace, "psi", psi);
      setGoniometer(input_workspace);
    }

    return convertToMD(input_workspace, emode, in_place, out_mdws);
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
