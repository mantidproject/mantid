#include "MantidAlgorithms/CalMuonDetectorPhases.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalMuonDetectorPhases)

// Define name for temporary workspace
const std::string CalMuonDetectorPhases::m_workspaceName = "CMDPInput";

//----------------------------------------------------------------------------------------------
/** Initializes the algorithm's properties.
 */
void CalMuonDetectorPhases::init() {

  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Name of the reference input workspace");

  declareProperty("FirstGoodData", EMPTY_DBL(),
                  "First good data point in units of micro-seconds",
                  Direction::Input);

  declareProperty("LastGoodData", EMPTY_DBL(),
                  "Last good data point in units of micro-seconds",
                  Direction::Input);

  declareProperty("Frequency", EMPTY_DBL(), "Starting hint for the frequency",
                  Direction::Input);

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "DetectorTable", "", Direction::Output),
                  "Name of the TableWorkspace in which to store the list "
                  "of phases and asymmetries");

  declareProperty(new API::WorkspaceProperty<API::WorkspaceGroup>(
                      "DataFitted", "", Direction::Output),
                  "Name of the output workspace holding fitting results");

  declareProperty(new ArrayProperty<int>("ForwardSpectra", Direction::Input),
                  "The spectra numbers of the forward group. If not specified "
                  "will read from file.");

  declareProperty(new ArrayProperty<int>("BackwardSpectra", Direction::Input),
                  "The spectra numbers of the backward group. If not specified "
                  "will read from file.");
}

//----------------------------------------------------------------------------------------------
/** Validates the inputs.
 */
std::map<std::string, std::string> CalMuonDetectorPhases::validateInputs() {

  std::map<std::string, std::string> result;

  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check units, should be microseconds
  Unit_const_sptr unit = inputWS->getAxis(0)->unit();
  if ((unit->label().ascii() != "Microseconds") &&
      (unit->label().ascii() != "microsecond")) {
    result["InputWorkspace"] = "InputWorkspace units must be microseconds";
  }

  // Check spectra numbers are valid, if specified
  int nspec = static_cast<int>(inputWS->getNumberHistograms());
  std::vector<int> forward = getProperty("ForwardSpectra");
  std::vector<int> backward = getProperty("BackwardSpectra");
  for (int spec : forward) {
    if (spec < 1 || spec > nspec) {
      result["ForwardSpectra"] = "Invalid spectrum numbers in ForwardSpectra";
    }
  }
  for (int spec : backward) {
    if (spec < 1 || spec > nspec) {
      result["BackwardSpectra"] = "Invalid spectrum numbers in BackwardSpectra";
    }
  }

  return result;
}
//----------------------------------------------------------------------------------------------
/** Executes the algorithm.
 */
void CalMuonDetectorPhases::exec() {

  // Get the input ws
  m_inputWS = getProperty("InputWorkspace");

  // Get start and end time
  double startTime = getStartTime();
  double endTime = getEndTime();

  // Get the frequency
  double freq = getFrequency();

  // Prepares the workspaces: extracts data from [startTime, endTime] and
  // removes exponential decay
  API::MatrixWorkspace_sptr tempWS = prepareWorkspace(startTime, endTime);

  // Create the output workspaces
  auto tab = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  auto group = API::WorkspaceGroup_sptr(new API::WorkspaceGroup());

  // Get the name of 'DataFitted'
  std::string groupName = getPropertyValue("DataFitted");

  // Fit the workspace
  fitWorkspace(tempWS, freq, groupName, tab, group);

  // Set the table
  setProperty("DetectorTable", tab);
  // Set the group
  setProperty("DataFitted", group);
}

/** Fits each spectrum in the workspace to f(x) = A * sin( w * x + p)
* @param ws :: [input] The workspace to fit
* @param freq :: [input] Hint for the frequency (w)
* @param groupName :: [input] The name of the output workspace group
* @param resTab :: [output] Table workspace storing the asymmetries and phases
* @param resGroup :: [output] Workspace group storing the fitting results
*/
void CalMuonDetectorPhases::fitWorkspace(const API::MatrixWorkspace_sptr &ws,
                                         double freq, std::string groupName,
                                         API::ITableWorkspace_sptr &resTab,
                                         API::WorkspaceGroup_sptr &resGroup) {

  int nspec = static_cast<int>(ws->getNumberHistograms());

  // Create the fitting function f(x) = A * sin ( w * x + p )
  std::string funcStr = createFittingFunction(freq);

  // Create the input string. Workspace must be in the ADS (temporarily)
  API::AnalysisDataService::Instance().addOrReplace(m_workspaceName, ws);
  std::ostringstream input;
  for (int i = 0; i < nspec; i++) {
    input << m_workspaceName << ",i" << i << ";";
  }

  auto fit = createChildAlgorithm("PlotPeakByLogValue");
  fit->initialize();
  fit->addObserver(this->progressObserver());
  setChildStartProgress(0.);
  setChildEndProgress(1.);
  fit->setPropertyValue("Function", funcStr);
  fit->setPropertyValue("Input", input.str());
  fit->setPropertyValue("OutputWorkspace", groupName);
  fit->setPropertyValue("FitType", "Individual"); // each fit starts with same initial parameters
  fit->setProperty("CreateOutput", true);
  fit->execute();

  // Get the fitting results - stored in ADS
  std::string resultsName(groupName);
  resGroup = boost::dynamic_pointer_cast<API::WorkspaceGroup>(
      API::AnalysisDataService::Instance().retrieve(
          resultsName.append("_Workspaces")));

  // Get the parameter table
  API::ITableWorkspace_sptr tab = fit->getProperty("OutputWorkspace");
  // Now we have our fitting results stored in tab
  // but we need to extract the relevant information, i.e.
  // the detector phases (parameter 'p') and asymmetries ('A')
  resTab = extractDetectorInfo(tab, static_cast<size_t>(nspec));

  // Clear temporary workspaces out of ADS
  clearUpADS(groupName);
}

/** Extracts detector asymmetries and phases from fitting results
* @param paramTab :: [input] Output parameter table resulting from the fit
* @param nspec :: [input] Number of detectors/spectra
* @return :: A new table workspace storing the asymmetries and phases
*/
API::ITableWorkspace_sptr CalMuonDetectorPhases::extractDetectorInfo(
    const API::ITableWorkspace_sptr &paramTab, size_t nspec) {

  // Make sure paramTable is the right size
  // It should contain as many rows as spectra
  if (paramTab->rowCount() != nspec) {
    throw std::invalid_argument(
        "Can't extract detector parameters from fit results");
  }

  // Create the table to store detector info
  auto tab = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  tab->addColumn("int", "Detector");
  tab->addColumn("double", "Asymmetry");
  tab->addColumn("double", "Phase");

  // Reference frequency, all w values should be the same
  double omegaRef = paramTab->Double(0, 3);

  for (size_t s = 0; s < nspec; s++) {
    // The following '3' factor corresponds to the number of function params
    double asym = paramTab->Double(s, 1);
    double omega = paramTab->Double(s, 3);
    double phase = paramTab->Double(s, 5);
    // If omega != omegaRef something went wrong with the fit
    if (omega != omegaRef) {
      throw std::runtime_error("Fit failed");
    }
    // If asym<0, take the absolute value and add \pi to phase
    // f(x) = A * sin( w * x + p) = -A * sin( w * x + p + PI)
    if (asym < 0) {
      asym = -asym;
      phase = phase + M_PI;
    }
    // Now convert phases to interval [0, 2PI)
    int factor = static_cast<int>(floor(phase / 2 / M_PI));
    if (factor) {
      phase = phase - factor * 2 * M_PI;
    }
    // Copy parameters to new table
    API::TableRow row = tab->appendRow();
    row << static_cast<int>(s) << asym << phase;
  }

  return tab;
}

/** Creates the fitting function f(x) = A * sin( w*x + p) as string
* @param freq :: [input] Fixed value for the frequency (w)
* @returns :: The fitting function as a string
*/
std::string CalMuonDetectorPhases::createFittingFunction(double freq) {

  // The fitting function is:
  // f(x) = A * sin ( w * x + p )
  // where w is shared across workspaces
  std::ostringstream ss;
  ss << "name=UserFunction,";
  ss << "Formula=A*sin(w*x+p),";
  ss << "A=0.5,";
  ss << "w=" << freq << ",";
  ss << "p=0.5;";
  ss << "ties=(f0.w=" << freq << ")";

  return ss.str();
}

/** Extracts relevant data from a workspace and removes exponential decay
* @param startTime :: [input] First X value to consider
* @param endTime :: [input] Last X value to consider
* @return :: Pre-processed workspace to fit
*/
API::MatrixWorkspace_sptr
CalMuonDetectorPhases::prepareWorkspace(double startTime, double endTime) {

  // Extract counts from startTime to endTime
  API::IAlgorithm_sptr crop = createChildAlgorithm("CropWorkspace");
  crop->setProperty("InputWorkspace", m_inputWS);
  crop->setProperty("XMin", startTime);
  crop->setProperty("XMax", endTime);
  crop->executeAsChildAlg();
  boost::shared_ptr<API::MatrixWorkspace> wsCrop =
      crop->getProperty("OutputWorkspace");

  // Remove exponential decay
  API::IAlgorithm_sptr remove = createChildAlgorithm("RemoveExpDecay");
  remove->setProperty("InputWorkspace", wsCrop);
  remove->executeAsChildAlg();
  boost::shared_ptr<API::MatrixWorkspace> wsRem =
      remove->getProperty("OutputWorkspace");

  return wsRem;
}

/**
 * Returns the frequency to use in the sequential fit.
 *
 * If user has provided a frequency as input, use that.
 * Otherwise, use 2*pi*g_mu*(sample_magn_field)
 *
 * @return :: Frequency to use in fit
 */
double CalMuonDetectorPhases::getFrequency() const {
  double freq = getProperty("Frequency");

  // If frequency is EMPTY_DBL():
  if (freq == EMPTY_DBL()) {
    try {
      // Read sample_magn_field from workspace logs
      freq = m_inputWS->run().getLogAsSingleValue("sample_magn_field");
      // Multiply by muon gyromagnetic ratio: 0.01355 MHz/G
      freq *= 2 * M_PI * PhysicalConstants::MuonGyromagneticRatio;
    } catch (...) {
      throw std::runtime_error(
          "Couldn't read sample_magn_field. Please provide a value for "
          "the frequency");
    }
  }
  return freq;
}

/**
 * Get start time for fit
 * If not provided as input, try to read from workspace logs.
 * If it's not there either, set to 0 and warn user.
 * @return :: Start time for fit
 */
double CalMuonDetectorPhases::getStartTime() const {
  double startTime = getProperty("FirstGoodData");
  // If startTime is EMPTY_DBL():
  if (startTime == EMPTY_DBL()) {
    try {
      // Read FirstGoodData from workspace logs if possible
      double firstGoodData =
          m_inputWS->run().getLogAsSingleValue("FirstGoodData");
      startTime = firstGoodData;
    } catch (...) {
      g_log.warning("Couldn't read FirstGoodData, setting to 0");
      // Set startTime to 0
      startTime = 0.;
    }
  }
  return startTime;
}

/**
 * Get end time for fit
 * If it's not there, use the last available time in the workspace.
 * @return :: End time for fit
 */
double CalMuonDetectorPhases::getEndTime() const {
  double endTime = getProperty("LastGoodData");
  // If endTime is EMPTY_DBL():
  if (endTime == EMPTY_DBL()) {
    // Last available time
    endTime = m_inputWS->readX(0).back();
  }
  return endTime;
}

/**
 * Removes temporary workspaces from the ADS
 */
void CalMuonDetectorPhases::clearUpADS(const std::string &groupName) const {
  std::ostringstream workspaces, covariance, parameters;
  API::AnalysisDataService::Instance().remove(m_workspaceName);
  workspaces << groupName << "_Workspaces";
  API::AnalysisDataService::Instance().deepRemoveGroup(workspaces.str());
  covariance << groupName << "_NormalisedCovarianceMatrices";
  API::AnalysisDataService::Instance().deepRemoveGroup(covariance.str());
  parameters << groupName << "_Parameters";
  API::AnalysisDataService::Instance().deepRemoveGroup(parameters.str());
}

} // namespace Algorithms
} // namespace Mantid
