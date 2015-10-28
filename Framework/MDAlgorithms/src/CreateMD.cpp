#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/CreateMD.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

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
  // TODO Auto-generated execute stub
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
Mantid::API::Workspace_sptr CreateMD::merge_runs(const std::string &to_merge) {
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

  std::vector<std::string> goniometer_params;
  goniometer_params.push_back(psi);
  goniometer_params.push_back(gl);
  goniometer_params.push_back(gs);

  std::vector<std::string> ub_params;
  ub_params.push_back(alatt);
  ub_params.push_back(angdeg);
  ub_params.push_back(u);
  ub_params.push_back(v);

  if (std::any_of(ub_params.begin(), ub_params.end(),
                  [](const std::string &param) { return param.empty(); }) &&
      !std::all_of(ub_params.begin(), ub_params.end(),
                   [](const std::string &param) { return param.empty(); })) {
    throw std::invalid_argument(
        "Either specify all of alatt, angledeg, u, v or none of them");
  } else if (std::all_of(
                 ub_params.begin(), ub_params.end(),
                 [](const std::string &param) { return param.empty(); })) {
    if (true) { // TODO check if UB set already
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

    Mantid::API::Workspace_sptr output_run =
        convertToMD(input_workspace, emode, in_place, out_mdws);
    return output_run;
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
