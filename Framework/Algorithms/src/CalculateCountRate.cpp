// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateCountRate.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <numeric>

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateCountRate)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateCountRate::name() const { return "CalculateCountRate"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculateCountRate::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateCountRate::category() const {
  return "Inelastic\\Utility;Diagnostics;Events\\EventFiltering";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateCountRate::summary() const {
  return "Calculates instrument count rate as the function of the "
         "experiment time and adds CountRate log to the source workspace.";
}

//----------------------------------------------------------------------------------------------
/** Declare the algorithm's properties.
 */
void CalculateCountRate::init() {

  declareProperty(
      std::make_unique<API::WorkspaceProperty<DataObjects::EventWorkspace>>("Workspace", "", Kernel::Direction::InOut),
      "Name of the event workspace to calculate counting rate for.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>("XMin", EMPTY_DBL(), Kernel::Direction::Input),
                  "Minimal value of X-range for the rate calculations. If left "
                  "to default, Workspace X-axis minimal value is used.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>("XMax", EMPTY_DBL(), Kernel::Direction::Input),
                  "Maximal value of X-range for the rate calculations. If left "
                  "to default, Workspace X-axis maximal value is used.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<std::string>>(
                      "RangeUnits", "Energy",
                      std::make_shared<Kernel::StringListValidator>(Kernel::UnitFactory::Instance().getKeys()),
                      Kernel::Direction::Input),
                  "The units from Mantid Unit factory for calculating the "
                  "counting rate and XMin-XMax ranges are in. If the "
                  "X-axis of the input workspace is not expressed"
                  "in these units, unit conversion will be performed, so the "
                  "workspace should contain all necessary information for this "
                  "conversion. E.g. if *RangeUnits* is *EnergyTransfer*, Ei "
                  "log containing incident energy value should be attached to the "
                  "input workspace. See ConvertUnits algorithm for the details.");
  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic", std::make_shared<Kernel::StringListValidator>(propOptions),
                  "The energy mode for 'RangeUnits' conversion mode (default: elastic)");

  // Used logs group
  std::string used_logs_mode("Used normalization logs");
  declareProperty("NormalizeTheRate", true,
                  "Usually you want to normalize counting rate to some "
                  "rate related to the source beam intensity. Change this to "
                  "'false' if appropriate time series log is broken || not attached to "
                  "the input workspace.");
  declareProperty("UseLogDerivative", false,
                  "If the normalization log contains "
                  "cumulative counting, derivative "
                  "of this log is necessary to get "
                  "correct normalization values.");
  declareProperty("NormalizationLogName", "proton_charge",
                  "The name of the log, used in the counting rate normalization. ");
  setPropertyGroup("NormalizeTheRate", used_logs_mode);
  setPropertyGroup("UseLogDerivative", used_logs_mode);
  setPropertyGroup("NormalizationLogName", used_logs_mode);

  // Results
  declareProperty("CountRateLogName", "block_count_rate", std::make_shared<Kernel::MandatoryValidator<std::string>>(),
                  "The name of the processed time series log with instrument "
                  "count rate to be added"
                  " to the source workspace");
  declareProperty("UseNormLogGranularity", true,
                  "If true, the count rate log will have the normalization log "
                  "accuracy; If false, the 'NumTimeSteps' in the visualization "
                  "workspace below will be used for the target log granularity too.");

  // visualization group
  std::string spur_vis_mode("Spurion visualization");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("VisualizationWs", "", Kernel::Direction::Output,
                                                             API::PropertyMode::Optional),
                  "Optional name to build 2D matrix workspace for spurion visualization. "
                  "If name is provided, a 2D workspace with this name will be created "
                  "containing data to visualize counting rate as function of time in the "
                  "ranges XMin-XMax");

  auto mustBeReasonable = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBeReasonable->setLower(3);
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<int>>("NumTimeSteps", 200, mustBeReasonable, Kernel::Direction::Input),
      "Number of time steps (time accuracy) the visualization workspace has. "
      "Also number of steps in 'CountRateLogName' log if "
      "'UseNormLogGranularity' is set to false. Should be bigger than 3");
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<int>>("XResolution", 100, mustBeReasonable, Kernel::Direction::Input),
      "Number of steps (accuracy) of the visualization workspace has along "
      "X-axis. ");
  setPropertyGroup("VisualizationWs", spur_vis_mode);
  setPropertyGroup("NumTimeSteps", spur_vis_mode);
  setPropertyGroup("XResolution", spur_vis_mode);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateCountRate::exec() {

  DataObjects::EventWorkspace_sptr sourceWS = getProperty("Workspace");
  API::EventType et = sourceWS->getEventType();
  if (et == API::EventType::WEIGHTED_NOTIME) {
    throw std::runtime_error("Event workspace " + sourceWS->getName() +
                             " contains events without necessary frame "
                             "information. Can not process counting rate");
  }

  // Identify correct way to treat input logs and general properties of output
  // log
  this->setOutLogParameters(sourceWS);

  //-------------------------------------
  // identify ranges for count rate calculations and initiate source workspace
  this->setSourceWSandXRanges(sourceWS);

  // check if visualization workspace is necessary and if it is, prepare
  // visualization workspace to use
  this->checkAndInitVisWorkspace();

  // create results log and add it to the source workspace
  const std::string logname = getProperty("CountRateLogName");
  auto newlog = std::make_unique<Kernel::TimeSeriesProperty<double>>(logname);
  // calculate averages requested and modify results log
  this->calcRateLog(m_workingWS, newlog.get());
  sourceWS->mutableRun().addProperty(std::move(newlog), true);

  // clear up log derivative and existing log pointer (if any)
  // to avoid incorrect usage
  // at subsequent calls to the same algorithm.
  m_tmpLogHolder.reset();
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
void CalculateCountRate::calcRateLog(DataObjects::EventWorkspace_sptr &InputWorkspace,
                                     Kernel::TimeSeriesProperty<double> *const targLog) {

  MantidVec countRate(m_numLogSteps);
  std::vector<double> countNormalization;
  if (this->normalizeCountRate())
    countNormalization = m_pNormalizationLog->valuesAsVector();

  std::unique_ptr<std::mutex[]> pVisWS_locks;
  if (this->buildVisWS()) {
    pVisWS_locks.reset(new std::mutex[m_visWs->getNumberHistograms()]);
  }

  auto nHist = static_cast<int64_t>(InputWorkspace->getNumberHistograms());
  // Initialize progress reporting.
  API::Progress prog(this, 0.0, 1.0, nHist);

  auto dTRangeMin = static_cast<double>(m_TRangeMin.totalNanoseconds());
  auto dTRangeMax = static_cast<double>(m_TRangeMax.totalNanoseconds());
  // The variable is used by a parallel code so this behaviour is desired
  // cppcheck-suppress variableScope
  std::vector<MantidVec> Buff;

#pragma omp parallel
  {
    int nThreads = PARALLEL_NUMBER_OF_THREADS;
#pragma omp single
    {
      // initialize thread's histogram buffer
      Buff.resize(nThreads);
    }
    auto nThread = PARALLEL_THREAD_NUMBER;
    Buff[nThread].assign(m_numLogSteps, 0);
#pragma omp for
    for (int64_t i = 0; i < nHist; ++i) {
      const auto loopThread = PARALLEL_THREAD_NUMBER;
      PARALLEL_START_INTERRUPT_REGION

      // Get a const event list reference. eventInputWS->dataY() doesn't work.
      const DataObjects::EventList &el = InputWorkspace->getSpectrum(i);
      el.generateCountsHistogramPulseTime(dTRangeMin, dTRangeMax, Buff[loopThread], m_XRangeMin, m_XRangeMax);
      if (this->buildVisWS()) {
        this->histogramEvents(el, pVisWS_locks.get());
      }

      // Report progress
      prog.report(name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
// calculate final sums
#pragma omp for
    for (int64_t j = 0; j < m_numLogSteps; j++) {

      for (int i = 0; i < nThreads; i++) {
        countRate[j] += Buff[i][j];
      }
      // normalize if requested
      if (!countNormalization.empty()) {
        countRate[j] /= countNormalization[j];
      }
    }
    if (!countNormalization.empty() && this->buildVisWS()) {
#pragma omp for
      for (int64_t j = 0; j < int64_t(m_visNorm.size()); j++) {
        m_visWs->mutableY(j) /= m_visNorm[j];
      }
    }
  }

  // generate target log timing
  std::vector<Types::Core::DateAndTime> times(m_numLogSteps);
  double dt = (dTRangeMax - dTRangeMin) / static_cast<double>(m_numLogSteps);
  auto t0 = m_TRangeMin.totalNanoseconds();
  for (auto i = 0; i < m_numLogSteps; i++) {
    times[i] = Types::Core::DateAndTime(t0 + static_cast<int64_t>((0.5 + double(i)) * dt));
  }
  // store calculated values within the target log.
  targLog->replaceValues(times, countRate);
}
/** histogram event list into visualization workspace
 * @param el       :: event list to rebin into visualization workspace
 * @param spectraLocks :: pointer to the array of mutexes to lock modifyed
 *                        visualization workspace spectra for a thread
 */
void CalculateCountRate::histogramEvents(const DataObjects::EventList &el, std::mutex *spectraLocks) {

  if (el.empty())
    return;

  auto events = el.getEvents();
  for (const Types::Event::TofEvent &ev : events) {
    double pulsetime = static_cast<double>(ev.pulseTime().totalNanoseconds());
    double tof = ev.tof();
    if (pulsetime < m_visT0 || pulsetime >= m_visTmax)
      continue;
    if (tof < m_XRangeMin || tof >= m_XRangeMax)
      continue;

    auto n_spec = static_cast<size_t>((pulsetime - m_visT0) / m_visDT);
    auto n_bin = static_cast<size_t>((tof - m_XRangeMin) / m_visDX);
    (spectraLocks + n_spec)->lock();
    auto &Y = m_visWs->mutableY(n_spec);
    Y[n_bin]++;
    (spectraLocks + n_spec)->unlock();
  }
}

/** Disable normalization using normalization log.
Helper function to avoid code duplication.
@param NormLogError -- error to print if normalization log is disabled*/
void CalculateCountRate::disableNormalization(const std::string &NormLogError) {
  g_log.warning() << NormLogError << std::endl;
  m_pNormalizationLog = nullptr;
  m_normalizeResult = false;
}
/*Analyse input log parameters and logs, attached to the workspace and identify
 * the parameters of the target log, including experiment time.
 *
 @param InputWorkspace -- input workspace to analyse logs
 */
void CalculateCountRate::setOutLogParameters(const DataObjects::EventWorkspace_sptr &InputWorkspace) {

  std::string NormLogName = getProperty("NormalizationLogName");
  std::string TargetLog = getProperty("CountRateLogName");
  if (NormLogName == TargetLog) {
    throw std::invalid_argument("Target log name: " + TargetLog + " and normalization log name: " + NormLogName +
                                " can not be the same");
  }

  m_normalizeResult = getProperty("NormalizeTheRate");
  bool useLogDeriv = getProperty("UseLogDerivative");
  bool useLogAccuracy = getProperty("UseNormLogGranularity");

  bool logPresent = InputWorkspace->run().hasProperty(NormLogName);
  if (!logPresent) {
    if (m_normalizeResult) {
      this->disableNormalization("Normalization log '" + NormLogName +
                                 "' values requested but the log is not attached to the "
                                 "workspace. Normalization disabled");
    }
    if (useLogDeriv) {
      g_log.warning() << "Normalization by log: '" << NormLogName
                      << "' -- log derivative requested but the source log is "
                         "not attached to "
                         "the workspace. Log derivative will not be used.\n";
      useLogDeriv = false;
    }
    if (useLogAccuracy) {
      g_log.warning() << "Using accuracy of the log: '" << NormLogName
                      << "' is requested but the log is not attached to the "
                         "workspace. Will use accuracy defined by "
                         "'NumTimeSteps' property value.\n";
      useLogAccuracy = false;
    }
  } else {
    m_pNormalizationLog = InputWorkspace->run().getTimeSeriesProperty<double>(NormLogName);
  }

  // Analyse properties interactions

  // if property derivative is specified.
  if (useLogDeriv) {
    m_tmpLogHolder = m_pNormalizationLog->getDerivative();
    m_pNormalizationLog = m_tmpLogHolder.get();
    m_useLogDerivative = true;
  }

  ///
  if (m_normalizeResult) {
    if (!useLogAccuracy) {
      g_log.warning() << "Change of the counting log accuracy while "
                         "normalizing by log values is not implemented. Will "
                         "use log accuracy.\n";
      useLogAccuracy = true;
    }
  } //---------------------------------------------------------------------
  // find target log ranges and identify what normalization should be used

  Types::Core::DateAndTime runTMin, runTMax;
  InputWorkspace->getPulseTimeMinMax(runTMin, runTMax);
  //
  if (useLogAccuracy) { // extract log times located inside the run time
    Types::Core::DateAndTime tLogMin, tLogMax;
    if (m_useLogDerivative) { // derivative moves events to the bin centre,
                              // but we need initial range
      auto pSource = InputWorkspace->run().getTimeSeriesProperty<double>(NormLogName);
      tLogMin = pSource->firstTime();
      tLogMax = pSource->lastTime();
    } else {
      tLogMin = m_pNormalizationLog->firstTime();
      tLogMax = m_pNormalizationLog->lastTime();
    }
    //
    if (tLogMin < runTMin || tLogMax > runTMax) {
      if (tLogMin > runTMax || tLogMax < runTMin) { // log time is outside of the experiment time.
                                                    // Log normalization is impossible
        this->disableNormalization("Normalization log " + m_pNormalizationLog->name() +
                                   " time lies outside of the whole experiment time. "
                                   "Log normalization impossible.");
        useLogAccuracy = false;
      } else {
        if (!m_tmpLogHolder) {
          m_tmpLogHolder = std::unique_ptr<Kernel::TimeSeriesProperty<double>>(m_pNormalizationLog->clone());
        }
        Kernel::TimeROI roi(runTMin, runTMax);
        if (Kernel::FilteredTimeSeriesProperty<double> *filteredLog =
                dynamic_cast<Kernel::FilteredTimeSeriesProperty<double> *>(m_tmpLogHolder.get())) {
          filteredLog->filterWith(roi);
          m_pNormalizationLog = filteredLog;
        } else {
          filteredLog = new Kernel::FilteredTimeSeriesProperty<double>(m_tmpLogHolder.get());
          filteredLog->filterWith(roi);
          m_pNormalizationLog = filteredLog;
        }

        m_numLogSteps = m_pNormalizationLog->size();
      }
    } else {
      if (tLogMin > runTMin || tLogMax < runTMax) {
        this->disableNormalization("Normalization log " + m_pNormalizationLog->name() +
                                   " time does not cover the whole experiment time. "
                                   "Log normalization impossible.");
        useLogAccuracy = false;
      }
    }
  }

  if (useLogAccuracy) {
    m_numLogSteps = m_pNormalizationLog->size();
    if (m_numLogSteps < 2) { // should not ever happen but...

      this->disableNormalization("Number of points in the Normalization log " + m_pNormalizationLog->name() +
                                 " smaller then 2. Can not normalize using this log.");
      m_numLogSteps = getProperty("NumTimeSteps"); // Always > 2
      useLogAccuracy = false;
    }
  } else {
    m_numLogSteps = getProperty("NumTimeSteps");
  }
  // identify epsilon to use with current time
  double t_epsilon = double(runTMax.totalNanoseconds()) * (1 + std::numeric_limits<double>::epsilon());
  auto eps_increment = static_cast<int64_t>(t_epsilon - double(runTMax.totalNanoseconds()));

  m_TRangeMin = runTMin - eps_increment;
  if (useLogAccuracy) {
    // Let's try to establish log step (it should be constant in real
    // applications) and define
    // binning in such a way, that each historgam bin accomodates
    // single log value.
    auto iTMax = runTMax.totalNanoseconds();
    auto iTMin = m_TRangeMin.totalNanoseconds();
    int64_t provDT = (iTMax - iTMin) / (m_numLogSteps - 1);
    if (provDT < 1) { // something is fundamentally wrong. This can only happen
                      // if the log is very short and the distance between log
                      // boundaries is smaller than dt
      this->disableNormalization("Time step of the log " + m_pNormalizationLog->name() +
                                 " is not consistent with number of log steps. "
                                 "Can not use this log normalization");
      useLogAccuracy = false;
    } else {
      auto iTMax1 = iTMin + provDT * m_numLogSteps;
      if (iTMax1 <= iTMax) { // == is possible
        m_numLogSteps++;
        iTMax1 = iTMin + provDT * m_numLogSteps;
      }
      m_TRangeMax = Types::Core::DateAndTime(iTMax1);
    }
  }

  if (!useLogAccuracy) {
    // histogramming excludes rightmost events. Modify max limit to keep them
    m_TRangeMax = runTMax + eps_increment; // Should be
    // *(1+std::numeric_limits<double>::epsilon())
    // but DateTime does not have multiplication
    // operator
  }
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
void CalculateCountRate::setSourceWSandXRanges(DataObjects::EventWorkspace_sptr &InputWorkspace) {

  std::string RangeUnits = getProperty("RangeUnits");
  auto axis = InputWorkspace->getAxis(0);
  const auto unit = axis->unit();

  API::MatrixWorkspace_sptr wst;
  if (unit->unitID() != RangeUnits) {
    auto conv = createChildAlgorithm("ConvertUnits", 0, 1);
    std::string wsName = InputWorkspace->getName();
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

    conv->setRethrows(true);
    conv->execute();
    wst = conv->getProperty("OutputWorkspace");

  } else {
    wst = InputWorkspace;
  }
  m_workingWS = std::dynamic_pointer_cast<DataObjects::EventWorkspace>(wst);
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
    // include rightmost limit into the histogramming
    m_XRangeMax = realMax * (1. + std::numeric_limits<double>::epsilon());
    return;
  }

  if (m_XRangeMin == EMPTY_DBL()) {
    m_XRangeMin = realMin;
  }
  if (m_XRangeMax == EMPTY_DBL()) {
    // include rightmost limit into the histogramming
    m_XRangeMax = realMax * (1. + std::numeric_limits<double>::epsilon());
  }
  if (m_XRangeMin < realMin) {
    g_log.debug() << "Workspace constrain min range changed from: " << m_XRangeMin << " To: " << realMin << std::endl;
    m_XRangeMin = realMin;
  }
  if (m_XRangeMax > realMax) {
    g_log.debug() << "Workspace constrain max range changed from: " << m_XRangeMax << " To: " << realMax << std::endl;
    m_XRangeMax = realMax * (1. + std::numeric_limits<double>::epsilon());
  }
  // check final ranges valid
  if (m_XRangeMax < realMin || m_XRangeMin > realMax) {
    throw std::invalid_argument(" Spurion data search range: [" + std::to_string(m_XRangeMin) + "," +
                                std::to_string(m_XRangeMin) + "] lies outside of the workspace's real data range: [" +
                                std::to_string(realMin) + "," + std::to_string(realMax) + "]");
  }

  if (m_XRangeMin > m_XRangeMax) {
    throw std::invalid_argument(" Minimal spurion search data limit is bigger "
                                "than the maximal limit. ( Min: " +
                                std::to_string(m_XRangeMin) + "> Max: " + std::to_string(m_XRangeMax) + ")");
  }
}

/**Check if visualization workspace is necessary and initiate it if requested.
 * Sets or clears up internal m_visWS pointer and "do-visualization workspace"
 * option.
 */
void CalculateCountRate::checkAndInitVisWorkspace() {
  std::string visWSName = getProperty("VisualizationWs");
  if (visWSName.empty()) {
    m_visWs.reset();
    m_doVis = false;
    return;
  }
  m_doVis = true;

  int numTBins = getProperty("NumTimeSteps");
  if (this->normalizeCountRate()) {
    if (numTBins > m_numLogSteps) {
      g_log.information() << "Number of time step in normalized visualization "
                             "workspace exceeds the number of points in the "
                             "normalization log. This mode is not supported so "
                             "number of time steps decreased to be equal to "
                             "the number of normalization log points\n";
      numTBins = m_numLogSteps;
    }
  }
  int numXBins = getProperty("XResolution");
  std::string RangeUnits = getProperty("RangeUnits");

  m_visWs = create<Workspace2D>(numTBins, BinEdges(numXBins + 1));
  m_visWs->setTitle(visWSName);

  double Xmax = m_XRangeMax;
  // a bit dodgy code. It can be generalized
  if (std::isinf(Xmax)) {
    const auto &Xbin = m_workingWS->x(0);
    for (int64_t i = Xbin.size() - 1; i >= 0; --i) {
      if (!std::isinf(Xbin[i])) {
        Xmax = Xbin[i];
        break;
      }
    }
    if (std::isinf(Xmax)) {
      g_log.warning() << "All X-range for visualization workspace is infinity. "
                         "Can not build visualization workspace in the units "
                         "requested\n";
      m_visWs.reset();
      m_doVis = false;
      return;
    }
  }

  // define X-axis in target units
  double dX = (Xmax - m_XRangeMin) / numXBins;
  std::vector<double> xx(numXBins);
  for (int i = 0; i < numXBins; ++i) {
    xx[i] = m_XRangeMin + (0.5 + static_cast<double>(i)) * dX;
  }
  auto ax0 = std::make_unique<API::NumericAxis>(xx);
  ax0->setUnit(RangeUnits);
  m_visWs->replaceAxis(0, std::move(ax0));

  // define Y axis (in seconds);
  double dt = (static_cast<double>(m_TRangeMax.totalNanoseconds() - m_TRangeMin.totalNanoseconds()) /
               static_cast<double>(numTBins)) *
              1.e-9;
  xx.resize(numTBins);
  for (int i = 0; i < numTBins; i++) {
    xx[i] = (0.5 + static_cast<double>(i)) * dt;
  }
  auto ax1 = std::make_unique<API::NumericAxis>(xx);
  auto labelY = std::dynamic_pointer_cast<Kernel::Units::Label>(Kernel::UnitFactory::Instance().create("Label"));
  labelY->setLabel("sec");
  ax1->unit() = labelY;
  m_visWs->replaceAxis(1, std::move(ax1));

  setProperty("VisualizationWs", m_visWs);

  // define binning parameters used while calculating visualization
  m_visX0 = m_XRangeMin;
  m_visDX = dX;
  m_visT0 = static_cast<double>(m_TRangeMin.totalNanoseconds());
  m_visTmax = static_cast<double>(m_TRangeMax.totalNanoseconds());
  m_visDT = (m_visTmax - m_visT0) / static_cast<double>(numTBins);

  if (this->normalizeCountRate()) {
    m_visNorm.resize(numTBins);
    this->buildVisWSNormalization(m_visNorm);
  }
}
/** Helper function to check if visualization workspace should be used*/
bool CalculateCountRate::buildVisWS() const { return m_doVis; }

/** Helper function, mainly for testing
 * @return  true if count rate should be normalized and false
 * otherwise */
bool CalculateCountRate::normalizeCountRate() const { return m_normalizeResult; }
/** Helper function, mainly for testing
 * @return  true if log derivative is used instead of log itself */
bool CalculateCountRate::useLogDerivative() const { return m_useLogDerivative; }

/** method to prepare normalization vector for the visualisation workspace using
* data from normalization log with, usually, different number of time steps
* Here we assume that the number of time points in the visualization workspace
* is smaller or equal to the number of points in the normalization log.
*
@param normalization -- on output, the vector containing normalization
*                       coefficients for the visualization workspace spectra
*/
void CalculateCountRate::buildVisWSNormalization(std::vector<double> &normalization) {
  if (!m_pNormalizationLog) {
    m_normalizeResult = false;
    g_log.warning() << "CalculateCountRate::buildVisWSNormalization: No source "
                       "normalization log is found. Will not normalize "
                       "visualization workspace\n";
    return;
  }
  // visualization workspace should be present and initialized at this stage:
  const auto ax = dynamic_cast<const API::NumericAxis *>(m_visWs->getAxis(1));
  if (!ax)
    throw std::runtime_error("Can not retrieve Y-axis from visualization workspace");

  normalization.assign(ax->length(), 0.);
  // For more accurate logging (in a future, if necessary:)
  // auto t_bins = ax->createBinBoundaries();
  // double dt = t_bins[2] - t_bins[1]; // equal bins, first bin may be weird.
  //
  m_pNormalizationLog->histogramData(m_TRangeMin, m_TRangeMax, normalization);
}

} // namespace Mantid::Algorithms
