#include "MantidAlgorithms/CalMuonDetectorPhases.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalMuonDetectorPhases)

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

  return result;
}
//----------------------------------------------------------------------------------------------
/** Executes the algorithm.
 */
void CalMuonDetectorPhases::exec() {

  // Get the input ws
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Get start and end time
  double startTime = getProperty("FirstGoodData");
  double endTime = getProperty("LastGoodData");

  // If startTime is EMPTY_DBL():
  if (startTime == EMPTY_DBL()) {
    try {
      // Read FirstGoodData from workspace logs if possible
      double firstGoodData =
          inputWS->run().getLogAsSingleValue("FirstGoodData");
      startTime = firstGoodData;
    } catch (...) {
      g_log.warning("Couldn't read FirstGoodData, setting to 0");
      // Set startTime to 0
      startTime = 0.;
    }
  }
  // If endTime is EMPTY_DBL():
  if (endTime == EMPTY_DBL()) {
    // Last available time
    endTime = inputWS->readX(0).back();
  }

  // Get the frequency
  double freq = getProperty("Frequency");

  // If frequency is EMPTY_DBL():
  if (freq == EMPTY_DBL()) {
    try {
      // Read sample_magn_field from workspace logs
      freq = inputWS->run().getLogAsSingleValue("sample_magn_field");
      // Multiply by muon gyromagnetic ratio: 0.01355 MHz/G
      freq *= 2 * M_PI * 0.01355;
    } catch (...) {
      throw std::runtime_error(
          "Couldn't read sample_magn_field. Please provide a value for "
          "the frequency");
    }
  }

  // Prepares the workspaces: extracts data from [startTime, endTime] and
  // removes exponential decay
  API::MatrixWorkspace_sptr tempWS =
      prepareWorkspace(inputWS, startTime, endTime);

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
  std::string funcStr = createFittingFunction(nspec, freq);
  // Create the function from string
  API::IFunction_sptr func =
      API::FunctionFactory::Instance().createInitialized(funcStr);
  // Create the multi domain function
  boost::shared_ptr<API::MultiDomainFunction> multi =
      boost::dynamic_pointer_cast<API::MultiDomainFunction>(func);
  // Set the domain indices
  for (int i = 0; i < nspec; ++i) {
    multi->setDomainIndex(i, i);
  }

  API::IAlgorithm_sptr fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->addObserver(this->progressObserver());
  setChildStartProgress(0.);
  setChildEndProgress(1.);
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<API::IFunction>(multi));
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", 0);
  for (int s = 1; s < nspec; s++) {
    std::string suffix = boost::lexical_cast<std::string>(s);
    fit->setProperty("InputWorkspace_" + suffix, ws);
    fit->setProperty("WorkspaceIndex_" + suffix, s);
  }
  fit->setProperty("CreateOutput", true);
  fit->setProperty("Output", groupName);
  fit->execute();

  // Get the parameter table
  API::ITableWorkspace_sptr tab = fit->getProperty("OutputParameters");
  // Now we have our fitting results stored in tab
  // but we need to extract the relevant information, i.e.
  // the detector phases (parameter 'p') and asymmetries ('A')
  resTab = extractDetectorInfo(tab, static_cast<size_t>(nspec));

  // Get the fitting results
  resGroup = fit->getProperty("OutputWorkspace");
}

/** Extracts detector asymmetries and phases from fitting results
* @param paramTab :: [input] Output parameter table resulting from the fit
* @param nspec :: [input] Number of detectors/spectra
* @return :: A new table workspace storing the asymmetries and phases
*/
API::ITableWorkspace_sptr CalMuonDetectorPhases::extractDetectorInfo(
    const API::ITableWorkspace_sptr &paramTab, size_t nspec) {

  // Make sure paramTable is the right size
  // It should contain three parameters per detector/spectrum plus 'const
  // function value'
  if (paramTab->rowCount() != nspec * 3 + 1) {
    throw std::invalid_argument(
        "Can't extract detector parameters from fit results");
  }

  // Create the table to store detector info
  auto tab = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  tab->addColumn("int", "Detector");
  tab->addColumn("double", "Asymmetry");
  tab->addColumn("double", "Phase");

  // Reference frequency, all w values should be the same
  double omegaRef = paramTab->Double(1, 1);

  for (size_t s = 0; s < nspec; s++) {
    // The following '3' factor corresponds to the number of function params
    size_t specRow = s * 3;
    double asym = paramTab->Double(specRow, 1);
    double omega = paramTab->Double(specRow + 1, 1);
    double phase = paramTab->Double(specRow + 2, 1);
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
* @param nspec :: [input] The number of domains (spectra)
* @param freq :: [input] Hint for the frequency (w)
* @returns :: The fitting function as a string
*/
std::string CalMuonDetectorPhases::createFittingFunction(int nspec,
                                                         double freq) {

  // The fitting function is:
  // f(x) = A * sin ( w * x + p )
  // where w is shared across workspaces
  std::ostringstream ss;
  ss << "composite=MultiDomainFunction,NumDeriv=true;";
  for (int s = 0; s < nspec; s++) {
    ss << "name=UserFunction,";
    ss << "Formula=A*sin(w*x+p),";
    ss << "A=0.5,";
    ss << "w=" << freq << ",";
    ss << "p=0.;";
  }
  ss << "ties=(";
  for (int s = 1; s < nspec - 1; s++) {
    ss << "f";
    ss << boost::lexical_cast<std::string>(s);
    ss << ".w=f0.w,";
  }
  ss << "f";
  ss << boost::lexical_cast<std::string>(nspec - 1);
  ss << ".w=f0.w)";

  return ss.str();
}

/** Extracts relevant data from a workspace and removes exponential decay
* @param ws :: [input] The input workspace
* @param startTime :: [input] First X value to consider
* @param endTime :: [input] Last X value to consider
* @return :: Pre-processed workspace to fit
*/
API::MatrixWorkspace_sptr
CalMuonDetectorPhases::prepareWorkspace(const API::MatrixWorkspace_sptr &ws,
                                        double startTime, double endTime) {

  // Extract counts from startTime to endTime
  API::IAlgorithm_sptr crop = createChildAlgorithm("CropWorkspace");
  crop->setProperty("InputWorkspace", ws);
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

} // namespace Algorithms
} // namespace Mantid
