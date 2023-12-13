// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/CalMuonDetectorPhases.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"

namespace {
int PHASE_ROW = 2;
double ASYMM_ERROR = 999.0;
} // namespace

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalMuonDetectorPhases)

/** Initializes the algorithm's properties.
 */
void CalMuonDetectorPhases::init() {

  declareProperty(std::make_unique<API::WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Name of the reference input workspace");

  declareProperty("FirstGoodData", EMPTY_DBL(), "First good data point in units of micro-seconds", Direction::Input);

  declareProperty("LastGoodData", EMPTY_DBL(), "Last good data point in units of micro-seconds", Direction::Input);

  declareProperty("Frequency", EMPTY_DBL(), "Starting hint for the frequency in MHz", Direction::Input);

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("DetectorTable", "", Direction::Output),
      "Name of the TableWorkspace in which to store the list "
      "of phases and asymmetries");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>("DataFitted", "", Direction::Output),
                  "Name of the output workspace holding fitting results");

  declareProperty(std::make_unique<ArrayProperty<int>>("ForwardSpectra", Direction::Input),
                  "The spectra numbers of the forward group. If not specified "
                  "will read from file.");

  declareProperty(std::make_unique<ArrayProperty<int>>("BackwardSpectra", Direction::Input),
                  "The spectra numbers of the backward group. If not specified "
                  "will read from file.");
}

/** Validates the inputs.
 */
std::map<std::string, std::string> CalMuonDetectorPhases::validateInputs() {

  std::map<std::string, std::string> result;

  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  if (inputWS) {
    // Check units, should be microseconds
    Unit_const_sptr unit = inputWS->getAxis(0)->unit();
    if ((unit->label().ascii() != "Microseconds") && (unit->label().ascii() != "microsecond")) {
      result["InputWorkspace"] = "InputWorkspace units must be microseconds";
    }

    // Check spectra numbers are valid, if specified
    auto nspec = static_cast<int>(inputWS->getNumberHistograms());
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

  // Prepares the workspaces: extracts data from [startTime, endTime]
  API::MatrixWorkspace_sptr tempWS = extractDataFromWorkspace(startTime, endTime);

  // Get the frequency
  double freq = getFrequency(tempWS);

  // Create the output workspaces
  TableWorkspace_sptr tab = std::make_shared<TableWorkspace>();
  auto group = std::make_shared<API::WorkspaceGroup>();

  // Get the name of 'DataFitted'
  std::string groupName = getPropertyValue("DataFitted");

  // Remove exponential decay and fit the workspace
  auto wsToFit = removeExpDecay(tempWS);
  fitWorkspace(wsToFit, freq, groupName, tab, group);

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
void CalMuonDetectorPhases::fitWorkspace(const API::MatrixWorkspace_sptr &ws, double freq, const std::string &groupName,
                                         const API::ITableWorkspace_sptr &resTab, API::WorkspaceGroup_sptr &resGroup) {

  auto nhist = static_cast<int>(ws->getNumberHistograms());

  // Create the fitting function f(x) = A * sin ( w * x + p )
  // The same function and initial parameters are used for each fit
  std::string funcStr = createFittingFunction(freq, true);

  // Set up results table
  resTab->addColumn("int", "Spectrum number");
  resTab->addColumn("double", "Asymmetry");
  resTab->addColumn("double", "Phase");

  const auto &indexInfo = ws->indexInfo();

  // Loop through fitting all spectra individually
  const static std::string success = "success";
  for (int wsIndex = 0; wsIndex < nhist; wsIndex++) {
    reportProgress(wsIndex, nhist);
    const auto &yValues = ws->y(wsIndex);
    auto emptySpectrum = std::all_of(yValues.begin(), yValues.end(), [](double value) { return value == 0.; });
    if (emptySpectrum) {
      g_log.warning("Spectrum " + std::to_string(wsIndex) + " is empty");
      TableWorkspace_sptr tab = std::make_shared<TableWorkspace>();
      tab->addColumn("str", "Name");
      tab->addColumn("double", "Value");
      tab->addColumn("double", "Error");
      for (int j = 0; j < 4; j++) {
        API::TableRow row = tab->appendRow();
        if (j == PHASE_ROW) {
          row << "dummy" << 0.0 << 0.0;
        } else {
          row << "dummy" << ASYMM_ERROR << 0.0;
        }
      }

      extractDetectorInfo(*tab, *resTab, indexInfo.spectrumNumber(wsIndex));

    } else {
      auto fit = createChildAlgorithm("Fit");
      fit->initialize();
      fit->setPropertyValue("Function", funcStr);
      fit->setProperty("InputWorkspace", ws);
      fit->setProperty("WorkspaceIndex", wsIndex);
      fit->setProperty("CreateOutput", true);
      fit->setPropertyValue("Output", groupName);
      fit->execute();

      std::string status = fit->getProperty("OutputStatus");
      if (!fit->isExecuted()) {
        std::ostringstream error;
        error << "Fit failed for spectrum at workspace index " << wsIndex;
        error << ": " << status;
        throw std::runtime_error(error.str());
      } else if (status != success) {
        g_log.warning("Fit failed for spectrum at workspace index " + std::to_string(wsIndex) + ": " + status);
      }

      API::MatrixWorkspace_sptr fitOut = fit->getProperty("OutputWorkspace");
      resGroup->addWorkspace(fitOut);
      API::ITableWorkspace_sptr tab = fit->getProperty("OutputParameters");
      // Now we have our fitting results stored in tab
      // but we need to extract the relevant information, i.e.
      // the detector phases (parameter 'p') and asymmetries ('A')
      extractDetectorInfo(*tab, *resTab, indexInfo.spectrumNumber(wsIndex));
    }
  }
}

/** Extracts detector asymmetries and phases from fitting results
 * and adds a new row to the results table with them
 * @param paramTab :: [input] Output parameter table resulting from the fit
 * @param resultsTab :: [input] Results table to update with a new row
 * @param spectrumNumber :: [input] Spectrum number
 */
void CalMuonDetectorPhases::extractDetectorInfo(API::ITableWorkspace &paramTab, API::ITableWorkspace &resultsTab,
                                                const Indexing::SpectrumNumber spectrumNumber) {

  double asym = paramTab.Double(0, 1);
  double phase = paramTab.Double(2, 1);
  // If asym<0, take the absolute value and add \pi to phase
  // f(x) = A * cos( w * x - p) = -A * cos( w * x - p - PI)
  if (asym < 0) {
    asym = -asym;
    phase = phase - M_PI;
  }
  // Now convert phases to interval [0, 2PI)
  auto factor = static_cast<int>(floor(phase / (2. * M_PI)));
  if (factor) {
    phase = phase - factor * 2. * M_PI;
  }
  // Copy parameters to new row in results table
  API::TableRow row = resultsTab.appendRow();
  row << static_cast<int>(spectrumNumber) << asym << phase;
}

/** Creates the fitting function f(x) = A * cos( w*x - p) + B as string
 * Two modes:
 * 1) Fixed frequency, no background - for main sequential fit
 * 2) Varying frequency, flat background - for finding frequency from asymmetry
 * @param freq :: [input] Value for the frequency (w)
 * @param fixFreq :: [input] True: fixed frequency, no background. False:
 * varying frequency with flat background.
 * @returns :: The fitting function as a string
 */
std::string CalMuonDetectorPhases::createFittingFunction(double freq, bool fixFreq) {
  // The fitting function is:
  // f(x) = A * sin ( w * x - p ) [+ B]
  std::ostringstream ss;
  ss << "name=UserFunction,";
  if (fixFreq) {
    // no background
    ss << "Formula=A*cos(w*x-p),";
  } else {
    // flat background
    ss << "Formula=A*cos(w*x-p)+B,";
    ss << "B=0.5,";
  }
  ss << "A=0.5,";
  ss << "w=" << freq << ",";
  ss << "p=0.5;";
  if (fixFreq) {
    // w is shared across workspaces
    ss << "ties=(f0.w=" << freq << ")";
  }

  return ss.str();
}

/** Extracts relevant data from a workspace
 * @param startTime :: [input] First X value to consider
 * @param endTime :: [input] Last X value to consider
 * @return :: Pre-processed workspace to fit
 */
API::MatrixWorkspace_sptr CalMuonDetectorPhases::extractDataFromWorkspace(double startTime, double endTime) {
  // Extract counts from startTime to endTime
  auto crop = createChildAlgorithm("CropWorkspace");
  crop->setProperty("InputWorkspace", m_inputWS);
  crop->setProperty("XMin", startTime);
  crop->setProperty("XMax", endTime);
  crop->executeAsChildAlg();
  std::shared_ptr<API::MatrixWorkspace> wsCrop = crop->getProperty("OutputWorkspace");
  return wsCrop;
}

/**
 * Removes exponential decay from a workspace
 * @param wsInput :: [input] Workspace to work on
 * @return :: Workspace with decay removed
 */
API::MatrixWorkspace_sptr CalMuonDetectorPhases::removeExpDecay(const API::MatrixWorkspace_sptr &wsInput) {
  auto remove = createChildAlgorithm("RemoveExpDecay");
  remove->setProperty("InputWorkspace", wsInput);
  remove->executeAsChildAlg();
  API::MatrixWorkspace_sptr wsRem = remove->getProperty("OutputWorkspace");
  return wsRem;
}

/**
 * Returns the frequency hint to use as a starting point for finding the
 * frequency.
 *
 * If user has provided a frequency (MHz) as input, use that converted to
 * Mrad/s.
 * Otherwise, use 2*pi*g_mu*(sample_magn_field).
 * (2*pi to convert MHz to Mrad/s)
 *
 * @return :: Frequency hint to use in Mrad/s
 */
double CalMuonDetectorPhases::getFrequencyHint() const {
  double freq = getProperty("Frequency");

  if (freq == EMPTY_DBL()) {
    try {
      // Read sample_magn_field from workspace logs
      freq = m_inputWS->run().getLogAsSingleValue("sample_magn_field");
      // Multiply by muon gyromagnetic ratio: 0.01355 MHz/G
      freq *= PhysicalConstants::MuonGyromagneticRatio;
    } catch (...) {
      throw std::runtime_error("Couldn't read sample_magn_field. Please provide a value for "
                               "the frequency");
    }
  }

  // Convert from MHz to Mrad/s
  freq *= 2 * M_PI;

  return freq;
}

/**
 * Returns the frequency to use in the sequential fit.
 *
 * Finds this by grouping the spectra and calculating the asymmetry, then
 * fitting this to get the frequency.
 * The starting value for this fit is taken from the frequency hint or logs.
 * @param ws :: [input] Pointer to cropped workspace with exp decay removed
 * @return :: Fixed frequency value to use in the sequential fit
 */
double CalMuonDetectorPhases::getFrequency(const API::MatrixWorkspace_sptr &ws) {
  std::vector<int> forward = getProperty("ForwardSpectra");
  std::vector<int> backward = getProperty("BackwardSpectra");

  // If grouping not provided, read it from the instrument
  if (forward.empty() || backward.empty()) {
    getGroupingFromInstrument(ws, forward, backward);
  }

  // Calculate asymmetry
  const double alpha = getAlpha(ws, forward, backward);
  const API::MatrixWorkspace_sptr wsAsym = getAsymmetry(ws, forward, backward, alpha);

  // Fit an oscillating function, allowing frequency to vary
  double frequency = fitFrequencyFromAsymmetry(wsAsym);

  return frequency;
}

/**
 * If grouping was not provided, find the instrument from the input workspace
 * and read the default grouping from its IDF. Returns the forward and backward
 * groupings as arrays of integers.
 * @param ws :: [input] Workspace to find grouping for
 * @param forward :: [output] Forward spectrum indices for given instrument
 * @param backward :: [output] Backward spectrum indices for given instrument
 */
void CalMuonDetectorPhases::getGroupingFromInstrument(const API::MatrixWorkspace_sptr &ws, std::vector<int> &forward,
                                                      std::vector<int> &backward) {
  // make sure both arrays are empty
  forward.clear();
  backward.clear();

  const auto instrument = ws->getInstrument();
  auto loader = std::make_unique<API::GroupingLoader>(instrument);

  if (instrument->getName() == "MUSR" || instrument->getName() == "CHRONUS") {
    // Two possibilities for grouping - use workspace log
    auto fieldDir = ws->run().getLogData("main_field_direction");
    if (fieldDir) {
      loader = std::make_unique<API::GroupingLoader>(instrument, fieldDir->value());
    }
    if (!fieldDir) {
      throw std::invalid_argument("Cannot use default instrument grouping for MUSR "
                                  "as main field direction is unknown");
    }
  }

  // Load grouping and find forward, backward groups
  std::string fwdRange, bwdRange;
  const auto grouping = loader->getGroupingFromIDF();
  size_t nGroups = grouping->groups.size();
  for (size_t iGroup = 0; iGroup < nGroups; iGroup++) {
    const std::string groupName = grouping->groupNames[iGroup];
    if (groupName == "fwd" || groupName == "left") {
      fwdRange = grouping->groups[iGroup];
    } else if (groupName == "bwd" || groupName == "bkwd" || groupName == "right") {
      bwdRange = grouping->groups[iGroup];
    }
  }

  // Use ArrayProperty's functionality to convert string ranges to groups
  this->setProperty("ForwardSpectra", fwdRange);
  this->setProperty("BackwardSpectra", bwdRange);
  forward = getProperty("ForwardSpectra");
  backward = getProperty("BackwardSpectra");
}

/**
 * Get start time for fit
 * If not provided as input, try to read from workspace logs.
 * If it's not there either, set to 0 and warn user.
 * @return :: Start time for fit
 */
double CalMuonDetectorPhases::getStartTime() const {
  double startTime = getProperty("FirstGoodData");
  if (startTime == EMPTY_DBL()) {
    try {
      // Read FirstGoodData from workspace logs if possible
      double firstGoodData = m_inputWS->run().getLogAsSingleValue("FirstGoodData");
      startTime = firstGoodData;
    } catch (...) {
      g_log.warning("Couldn't read FirstGoodData, setting to 0");
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
  if (endTime == EMPTY_DBL()) {
    // Last available time
    endTime = m_inputWS->readX(0).back();
  }
  return endTime;
}

/**
 * Calculate alpha (detector efficiency) from the given workspace
 * If calculation fails, returns default 1.0
 * @param ws :: [input] Workspace to calculate alpha from
 * @param forward :: [input] Forward group spectra numbers
 * @param backward :: [input] Backward group spectra numbers
 * @return :: Alpha, or 1.0 if calculation failed
 */
double CalMuonDetectorPhases::getAlpha(const API::MatrixWorkspace_sptr &ws, const std::vector<int> &forward,
                                       const std::vector<int> &backward) {
  double alpha = 1.0;
  try {
    auto alphaAlg = createChildAlgorithm("AlphaCalc");
    alphaAlg->setProperty("InputWorkspace", ws);
    alphaAlg->setProperty("ForwardSpectra", forward);
    alphaAlg->setProperty("BackwardSpectra", backward);
    alphaAlg->executeAsChildAlg();
    alpha = alphaAlg->getProperty("Alpha");
  } catch (const std::exception &e) {
    // Eat the error and return default 1.0 so algorithm can continue.
    // Warn the user that calculating alpha failed
    std::ostringstream message;
    message << "Calculating alpha failed, default to 1.0: " << e.what();
    g_log.error(message.str());
  }
  return alpha;
}

/**
 * Calculate asymmetry for the given workspace
 * @param ws :: [input] Workspace to calculate asymmetry from
 * @param forward :: [input] Forward group spectra numbers
 * @param backward :: [input] Backward group spectra numbers
 * @param alpha :: [input] Detector efficiency
 * @return :: Asymmetry for workspace
 */
API::MatrixWorkspace_sptr CalMuonDetectorPhases::getAsymmetry(const API::MatrixWorkspace_sptr &ws,
                                                              const std::vector<int> &forward,
                                                              const std::vector<int> &backward, const double alpha) {
  auto alg = createChildAlgorithm("AsymmetryCalc");
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("OutputWorkspace", "__NotUsed");
  alg->setProperty("ForwardSpectra", forward);
  alg->setProperty("BackwardSpectra", backward);
  alg->setProperty("Alpha", alpha);
  alg->executeAsChildAlg();
  API::MatrixWorkspace_sptr wsAsym = alg->getProperty("OutputWorkspace");
  return wsAsym;
}

/**
 * Fit the asymmetry and return the frequency found.
 * Starting value for the frequency is taken from the hint.
 * If the fit fails, return the initial hint.
 * @param wsAsym :: [input] Workspace with asymmetry to fit
 * @return :: Frequency found from fit
 */
double CalMuonDetectorPhases::fitFrequencyFromAsymmetry(const API::MatrixWorkspace_sptr &wsAsym) {
  // Starting value for frequency is hint
  double hint = getFrequencyHint();
  std::string funcStr = createFittingFunction(hint, false);
  double frequency = hint;

  std::string fitStatus = "success";
  try {
    auto func = API::FunctionFactory::Instance().createInitialized(funcStr);
    auto fit = createChildAlgorithm("Fit");
    fit->setProperty("Function", func);
    fit->setProperty("InputWorkspace", wsAsym);
    fit->setProperty("WorkspaceIndex", 0);
    fit->setProperty("CreateOutput", true);
    fit->setProperty("OutputParametersOnly", true);
    fit->setProperty("Output", "__Invisible");
    fit->executeAsChildAlg();
    fitStatus = fit->getPropertyValue("OutputStatus");
    if (fitStatus == "success") {
      API::ITableWorkspace_sptr params = fit->getProperty("OutputParameters");
      const size_t rows = params->rowCount();
      static size_t colName(0), colValue(1);
      for (size_t iRow = 0; iRow < rows; iRow++) {
        if (params->cell<std::string>(iRow, colName) == "w") {
          frequency = params->cell<double>(iRow, colValue);
          break;
        }
      }
    }
  } catch (const std::exception &e) {
    // Report fit failure to user
    fitStatus = e.what();
  }
  if (fitStatus != "success") { // Either failed, or threw an exception
    std::ostringstream message;
    message << "Fit failed (" << fitStatus << "), using omega hint = " << hint;
    g_log.error(message.str());
  }
  return frequency;
}

/**
 * Updates the algorithm progress
 * @param thisSpectrum :: [input] Spectrum number currently being fitted
 * @param totalSpectra :: [input] Total number of spectra to fit
 */
void CalMuonDetectorPhases::reportProgress(const int thisSpectrum, const int totalSpectra) {
  double proportionDone = (double)thisSpectrum / (double)totalSpectra;
  std::ostringstream progMessage;
  progMessage << "Fitting " << thisSpectrum + 1 << " of " << totalSpectra;
  this->progress(proportionDone, progMessage.str());
}

} // namespace Mantid::Algorithms
