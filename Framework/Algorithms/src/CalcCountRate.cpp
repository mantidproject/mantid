#include "MantidAlgorithms/CalcCountRate.h"

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/MandatoryValidator.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalcCountRate)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalcCountRate::name() const { return "CalcCountRate"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalcCountRate::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalcCountRate::category() const {
  return "Inelastic\\Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalcCountRate::summary() const {
  return "Calculates instrument count rate as the function of the "
         "experiment time and adds CountRate log to the source workspace.";
}

//----------------------------------------------------------------------------------------------
/** Declare the algorithm's properties.
 */
void CalcCountRate::init() {

  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<DataObjects::EventWorkspace>>(
          "Workspace", "", Kernel::Direction::InOut),
      "Name of the event workspace to calculate counting rate for.");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<double>>(
                      "XMin", EMPTY_DBL(), Kernel::Direction::Input),
                  "Minimal value of X-range for the rate calculations. If left "
                  "to default, Workspace X-axis minimal value is used.");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<double>>(
                      "XMax", EMPTY_DBL(), Kernel::Direction::Input),
                  "Maximal value of X-range for the rate calculations. If left "
                  "to default, Workspace X-axis maximal value is used.");
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<std::string>>(
          "RangeUnits", "Energy",
          boost::make_shared<Kernel::StringListValidator>(
              Kernel::UnitFactory::Instance().getKeys()),
          Kernel::Direction::Input),
      "The units from Mantid Unit factory for calculating the "
      "counting rate and XMin-XMax ranges are in. If the "
      "X-axis of the input workspace is not expressed"
      "in this units, unit conversion will be performed, so the "
      "workspace should contain all necessary information for this "
      "conversion. E.g. if *RangeUnits* is *EnergyTransfer*, Ei "
      "log containing incident energy value should be attached to the "
      "input workspace.");
  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic",
                  boost::make_shared<Kernel::StringListValidator>(propOptions),
                  "The range units above conversion mode (default: elastic)");

  // Used logs group
  std::string used_logs_mode("Used normalization logs");
  declareProperty(
      "NormalizeTheRate", true,
      "Usually you want to normalize counting rate to some "
      "rate related to the source beam intensity. Change this to "
      "'false' if appropriate time series log is broken || not attached to "
      "the input workspace.");
  declareProperty("UseLogDerivative", false, "If the normalization log gives "
                                             "cumulative counting, derivative "
                                             "of this log is necessary to get "
                                             "correct normalization values.");
  declareProperty(
      "NormalizationLogName", "proton_charge",
      "The name of the log, used in the counting rate normalization. ");
  declareProperty(
      "UseNormLogGranularity", true,
      "If true, the calculated log will have the normalization log "
      "accuracy; If false, the 'NumTimeSteps' in the visualization "
      "workspace below will be used for the target log granularity too.");
  setPropertyGroup("NormalizeTheRate", used_logs_mode);
  setPropertyGroup("UseLogDerivative", used_logs_mode);
  setPropertyGroup("NormalizationLogName", used_logs_mode);
  setPropertyGroup("UseNormLogGranularity", used_logs_mode);

  declareProperty(
      "CountRateLogName", "block_count_rate",
      boost::make_shared<Kernel::MandatoryValidator<std::string>>(),
      "The name of the processed time series log with count rate to be added"
      " to the source workspace");
  // visualisation group
  std::string spur_vis_mode("Spurion visualisation");
  declareProperty(
      "VisualizationWsName", "",
      "Optional name to build 2D matrix workspace for spurion visualization. "
      "If name is provided, a 2D workspace with this name will be created "
      "containing data to visualize counting rate as function of time in the "
      "ranges "
      "XMin-XMax");
  declareProperty(Kernel::make_unique <
                  API::WorkspaceProperty<>>("VisualizationWs", "",
                                           Kernel::Direction::Output,
                                           API::PropertyMode::Optional));

  auto mustBeReasonable = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBeReasonable->setLower(3);
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<int>>(
          "NumTimeSteps", 200, mustBeReasonable, Kernel::Direction::Input),
      "Number of time steps (time accuracy) the visualization workspace has. "
      "Also number of steps in 'CountRateLogName' log if "
      "'UseNormLogGranularity' is set to false. Should be bigger than 3");
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<int>>(
          "XResolution", 100, mustBeReasonable, Kernel::Direction::Input),
      "Number of steps (accuracy) of the visualization workspace has along "
      "X-axis. ");
  setPropertyGroup("VisualizationWsName", spur_vis_mode);
  setPropertyGroup("NumTimeSteps", spur_vis_mode);
  setPropertyGroup("XResolution", spur_vis_mode);
  setPropertyGroup("VisualizationWs", spur_vis_mode);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalcCountRate::exec() {

  DataObjects::EventWorkspace_sptr sourceWS = getProperty("Workspace");

  // Identity correct way to treat input logs and general properties of output
  // log
  this->setOutLogParameters(sourceWS);

  //-------------------------------------
  // identify ranges for count rate calculations and initiate source workspace
  this->setSourceWSandXRanges(sourceWS);

  // check if visualization workspace is necessary and if it is, prepare
  // visualization workspace to use
  this->checkAndInitVisWorkspace();

  // create results log and add it to the source workspace
  std::string logname = getProperty("CountRateLogName");
  auto newlog = new Kernel::TimeSeriesProperty<double>(logname);
  sourceWS->mutableRun().addProperty(newlog, true);

  // calculate averages requested and modify results log
  this->calcRateLog(m_workingWS, newlog);

  // clear up log derivative and existing log pointer (if any)
  // to avoid incorrect usage
  // at subsequent calls to the same algorithm.
  m_tmpLogHolder.release();
  m_pNormalizationLog = nullptr;
}

/** Process input workspace to calculate instrument counting rate as function of
*experiment time
*@param InputWorkspace :: shared pointer to the input workspace to process
*@param targLog        :: pointer to time series property containing count rate
*log.
*                         Property should exist on input and will be modified
*with
*                         counting rate log on output.
*/
void CalcCountRate::calcRateLog(
    DataObjects::EventWorkspace_sptr &InputWorkspace,
    Kernel::TimeSeriesProperty<double> *const targLog) {

  MantidVec countRate(m_numLogSteps);
  std::vector<double> countNormalization;

  if (this->notmalizeCountRate())
    countNormalization = m_pNormalizationLog->valuesAsVector();

  int64_t nHist = static_cast<int64_t>(InputWorkspace->getNumberHistograms());
  // Initialize progress reporting.
  API::Progress prog(this, 0.0, 1.0, nHist);

  double dTRangeMin = static_cast<double>(m_TRangeMin.totalNanoseconds());
  double dTRangeMax = static_cast<double>(m_TRangeMax.totalNanoseconds());
  std::vector<MantidVec> Buff;
  int nThreads;

#pragma omp parallel
  {
#pragma omp single
    {
      // initialize thread's histogram buffer
      nThreads = omp_get_num_threads();
      Buff.resize(nThreads);
      for (size_t i = 0; i < nThreads; i++) {
        Buff[i].assign(m_numLogSteps, 0);
      }
    }

#pragma omp for
    for (int64_t i = 0; i < nHist; ++i) {
      auto nThread = omp_get_thread_num();
      PARALLEL_START_INTERUPT_REGION

      // Get a const event list reference. eventInputWS->dataY() doesn't work.
      const DataObjects::EventList &el = InputWorkspace->getSpectrum(i);
      el.generateCountsHistogramPulseTime(dTRangeMin, dTRangeMax, Buff[nThread],
                                          m_XRangeMin, m_XRangeMax);

      // Report progress
      prog.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
// calculate final sums
#pragma omp for
    for (int64_t j = 0; j < m_numLogSteps; j++) {

      for (size_t i = 0; i < nThreads; i++) {
        countRate[j] += Buff[i][j];
      }
      // normalize if requested
      if (!countNormalization.empty()) {
        countRate[j] /= countNormalization[j];
      }
    }
  }
  // recover histogram timing, but start assuming run starts at
  std::vector<Kernel::DateAndTime> times(m_numLogSteps);

  double dt = (dTRangeMax - dTRangeMin) / m_numLogSteps;
  auto t0 = m_TRangeMin.totalNanoseconds();
  for (size_t i = 0; i < m_numLogSteps; i++) {
    times[i] = Kernel::DateAndTime(t0 + static_cast<int64_t>((0.5 + i) * dt));
  }
  // store calculated values within the target log.
  targLog->replaceValues(times, countRate);
}

/*Analyse input log parameters and logs, attached to the workspace and identify
 * the parameters of the target log, including experiment time.
 *
 @param InputWorkspace -- input workspace to analyse logs
 */
void CalcCountRate::setOutLogParameters(
    const DataObjects::EventWorkspace_sptr &InputWorkspace) {

  std::string NormLogName = getProperty("NormalizationLogName");

  bool normalizeResult = getProperty("NormalizeTheRate");
  bool useLogDerivative = getProperty("UseLogDerivative");
  bool useLogAccuracy = getProperty("UseNormLogGranularity");

  bool logPresent = InputWorkspace->run().hasProperty(NormLogName);
  if (!logPresent) {
    if (normalizeResult) {
      g_log.warning() << "Normalization by log " << NormLogName
                      << " values requested but the log is not attached to the "
                         "workspace. Normalization disabled\n";
      normalizeResult = false;
    }
    if (useLogDerivative) {
      g_log.warning() << "Normalization by log " << NormLogName
                      << " derivative requested but the log is not attached to "
                         "the workspace. Normalization disabled\n";
      useLogDerivative = false;
    }
    if (useLogAccuracy) {
      g_log.warning() << "Using accuracy of the log " << NormLogName
                      << " is requested but the log is not attached to the "
                         "workspace. Will use accuracy defined by "
                         "'NumTimeSteps' property value\n";
      useLogAccuracy = false;
    }
  } else {
    m_pNormalizationLog =
        InputWorkspace->run().getTimeSeriesProperty<double>(NormLogName);
  }

  // Analyse properties interactions

  // if property derivative is specified.
  if (useLogDerivative) {
    m_tmpLogHolder = m_pNormalizationLog->getDerivative();
    m_pNormalizationLog = m_tmpLogHolder.get();
    m_useLogDerivative = true;
  }

  ///
  if (normalizeResult) {
    if (!useLogAccuracy) {
      g_log.warning() << "Change of the counting log accuracy while "
                         "normalizing by log values is not implemented. Will "
                         "use log accuracy.";
      useLogAccuracy = true;
    }
  } //---------------------------------------------------------------------
  // find target log ranges and identify what normalization should be used

  Kernel::DateAndTime runTMin, runTMax;
  InputWorkspace->getPulseTimeMinMax(runTMin, runTMax);
  //
  if (useLogAccuracy) { // extract log times located inside the run time
    Kernel::DateAndTime tLogMin, tLogMax;
    if (m_useLogDerivative) { // derivative moves events to the bin center,
                              // but we need initial range
      auto pSource =
          InputWorkspace->run().getTimeSeriesProperty<double>(NormLogName);
      tLogMin = pSource->firstTime();
      tLogMax = pSource->lastTime();
    } else {
      tLogMin = m_pNormalizationLog->firstTime();
      tLogMax = m_pNormalizationLog->lastTime();
    }
    //
    if (tLogMin < runTMin || tLogMax > runTMax) {
      if (tLogMin > runTMax ||
          tLogMax < runTMin) { // log time is outside of the experiment time.
                               // Log normalization is impossible
        g_log.warning()
            << "Normalization log " << m_pNormalizationLog->name()
            << " time lies outside of the the whole experiment time. "
               "Log normalization impossible.\n";
        m_pNormalizationLog = nullptr;
        useLogAccuracy = false;
      } else {
        if (!m_tmpLogHolder) {
          m_tmpLogHolder = std::make_unique<Kernel::TimeSeriesProperty<double>>(
              *m_pNormalizationLog->clone());
        }
        m_tmpLogHolder->filterByTime(runTMin, runTMax);
        m_pNormalizationLog = m_tmpLogHolder.get();
        m_numLogSteps = m_pNormalizationLog->realSize();
      }
    } else {
      if (tLogMin > runTMin || tLogMax < runTMax) {
        g_log.warning() << "Normalization log " << m_pNormalizationLog->name()
                        << " time does not cover the whole experiment time. "
                           "Log normalization impossible.\n";
        m_pNormalizationLog = nullptr;
        useLogAccuracy = false;
      }
    }
  }

  if (!useLogAccuracy) {
    m_numLogSteps = getProperty("NumTimeSteps");
  }

  m_TRangeMin = runTMin;
  m_TRangeMax = runTMax;
}

/* Retrieve and define data search ranges from input workspace parameters and
 * algorithm properties
 *
 *@param InputWorkspace -- event workspace to process. Also retrieves algorithm
 *                         properties, relevant to the workspace.
 *@return -- the input workspace cropped according to XMin-XMax ranges in units,
 *           requested by the user
 *
*/
void CalcCountRate::setSourceWSandXRanges(
    DataObjects::EventWorkspace_sptr &InputWorkspace) {

  std::string RangeUnits = getProperty("RangeUnits");
  auto axis = InputWorkspace->getAxis(0);
  const auto unit = axis->unit();

  API::MatrixWorkspace_sptr wst;
  if (unit->unitID() != RangeUnits) {
    auto conv = createChildAlgorithm("ConvertUnits", 0, 1);
    std::string wsName = InputWorkspace->name();
    if (wsName.empty()) {
      wsName = "_CountRate_UnitsConverted";
    } else {
      wsName = "_" + wsName + "_converted";
    }

    conv->setProperty("InputWorkspace", InputWorkspace);
    conv->setPropertyValue("OutputWorkspace", wsName);
    std::string Emode = getProperty("Emode");
    conv->setProperty("Emode", Emode);
    conv->setProperty("Target", RangeUnits);

    conv->execute();
    wst = conv->getProperty("OutputWorkspace");

  } else {
    wst = InputWorkspace;
  }
  m_workingWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(wst);
  if (!m_workingWS) {
    throw std::runtime_error("SetWSDataRanges:Can not retrieve EventWorkspace "
                             "after converting units");
  }
  // data ranges
  m_XRangeMin = getProperty("XMin");
  m_XRangeMax = getProperty("XMax");

  if (m_XRangeMin == EMPTY_DBL() && m_XRangeMax == EMPTY_DBL()) {
    m_rangeExplicit = false;
  } else {
    m_rangeExplicit = true;
  }

  double realMin, realMax;
  m_workingWS->getEventXMinMax(realMin, realMax);
  if (!m_rangeExplicit) { // The range is the whole workspace range
    m_XRangeMin = realMin;
    m_XRangeMax = realMax;
    return;
  }

  if (m_XRangeMin == EMPTY_DBL()) {
    m_XRangeMin = realMin;
  }
  if (m_XRangeMax == EMPTY_DBL()) {
    m_XRangeMax = realMax;
  }
  if (m_XRangeMin < realMin) {
    g_log.debug() << "Workspace constrain min range changed from: "
                  << m_XRangeMin << " To: " << realMin << std::endl;
    m_XRangeMin = realMin;
  }
  if (m_XRangeMax > realMax) {
    g_log.debug() << "Workspace constrain max range changed from: "
                  << m_XRangeMax << " To: " << realMax << std::endl;
    m_XRangeMax = realMax;
  }
}

/**Check if visualization workspace is necessary and initiate it if requested.
* Sets or clears up internal m_visWS pointer and "do-visualization workspace"
* option.
*/
void CalcCountRate::checkAndInitVisWorkspace() {
  std::string visWSName = getProperty("VisualizationWsName");
  if (visWSName.empty()) {
    m_visWs.reset();
    m_doVis = false;
    return;
  }
  m_doVis = true;

  int numTBins = getProperty("NumTimeSteps");
  int numXBins = getProperty("XResolution");
  std::string RangeUnits = getProperty("RangeUnits");

  m_visWs = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numTBins,
                                               numXBins + 1, numXBins));
  m_visWs->setTitle(visWSName);

  double Xmax = m_XRangeMax;
  // a bit dodgy code. It can be generalized
  if (std::isinf(Xmax)) {
    auto Xbin = m_workingWS->readX(0);
    for (int64_t i = Xbin.size() - 1; i >= 0; --i) {
      if (!std::isinf(Xbin[i])) {
        Xmax = Xbin[i];
        break;
      }
    }
  }

  // define X-axis in target units
  double dX = (Xmax - m_XRangeMin) / numXBins;
  std::vector<double> xx(numXBins);
  for (size_t i = 0; i < numXBins; ++i) {
    xx[i] = m_XRangeMin + 0.5 * dX + i * dX;
  }
  auto ax0 = new API::NumericAxis(xx);
  ax0->setUnit(RangeUnits);
  m_visWs->replaceAxis(0, ax0);

  // define Y axis (in seconds);
  double dt =
      ((m_TRangeMax.totalNanoseconds() - m_TRangeMin.totalNanoseconds()) /
       numTBins) *
      1.e-9;
  xx.resize(numTBins);
  for (size_t i = 0; i < numTBins; i++) {
    xx[i] = 0.5 * dt + dt * i;
  }
  auto ax1 = new API::NumericAxis(xx);
  auto labelY = boost::dynamic_pointer_cast<Kernel::Units::Label>(
      Kernel::UnitFactory::Instance().create("Label"));
  labelY->setLabel("sec");
  ax1->unit() = labelY;
  m_visWs->replaceAxis(1, ax1);

  setProperty("VisualizationWs", m_visWs);
}
/** Helper function to check if visualization workspace should be used*/
bool CalcCountRate::buildVisWS() const { return m_doVis; }

/** Helper function, mainly for testing
* @return  true if count rate should be normalized and false
* otherwise */
bool CalcCountRate::notmalizeCountRate() const {
  return (m_pNormalizationLog != nullptr);
}
/** Helper function, mainly for testing
* @return  true if log derivative is used instead of log itself */
bool CalcCountRate::useLogDerivative() const { return m_useLogDerivative; }

} // namespace Algorithms
} // namespace Mantid
