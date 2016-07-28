#include "MantidAlgorithms/CalcCountRate.h"

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAPI/AlgorithmManager.h"

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
          "RangeUnits", "Energy", Kernel::Direction::Input),
      "The units from Mantid Unit factory for calculating the "
      "counting rate and XMin-XMax ranges are in. If the "
      "X-axis of the input workspace is not expressed"
      "in this units, unit conversion will be performed, so the "
      "workspace should contain all necessary information for this "
      "conversion. E.g. if *RangeUnits* is *EnergyTransfer*, Ei "
      "log containing incident energy value should be attached to the "
      "input workspace.");
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
      "The name of the processed time series log with count rate to add"
      " to the source workspace");
  // visualisation group
  std::string spur_vis_mode("Spurion visualisation");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<DataObjects::Workspace2D>>(
          "VisualizationWs", "", Kernel::Direction::Output,
          API::PropertyMode::Optional),
      "Optional workspace name to build workspace for spurion visualization. "
      "If name is provided, a 2D workspace with this name will be created "
      "containing workspace to visualize counting rate in the ranges "
      "XMin-XMax");

  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<int>>(
          "NumTimeSteps", 200, mustBePositive, Kernel::Direction::Input),
      "Number of time steps (time accuracy) the visualization workspace has. "
      "Also number of steps in 'CountRateLogName' log if "
      "'UseNormLogGranularity' is set to false");
  declareProperty(
      Kernel::make_unique<Kernel::PropertyWithValue<int>>(
          "XResolution", 100, mustBePositive, Kernel::Direction::Input),
      "Number of steps (accuracy) of the visualization workspace has along "
      "X-axis. ");
  setPropertyGroup("VisualizationWs", spur_vis_mode);
  setPropertyGroup("NumTimeSteps", spur_vis_mode);
  setPropertyGroup("XResolution", spur_vis_mode);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalcCountRate::exec() {

  DataObjects::EventWorkspace_sptr sourceWS = getProperty("Workspace");
  // Sum spectra of the input workspace
  auto summator =
      API::AlgorithmManager::Instance().createUnmanaged("SumSpectra");
  summator->initialize();
  summator->setChild(true);
  summator->setProperty("InputWorkspace", sourceWS);
  summator->execute();

  API::MatrixWorkspace_sptr source = summator->getProperty("OutputWorkspace");
  m_workingWS =
      boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(source);
  if (!m_workingWS) {
    throw std::invalid_argument(
        "Can not sum spectra of input event workspace: " + sourceWS->name());
  }
  //-------------------------------------
  this->getSearchRanges(m_workingWS);

  std::string logname = getProperty("CountRateLogName");
  auto newlog = new Kernel::TimeSeriesProperty<double>(logname);
  sourceWS->mutableRun().addProperty(newlog, true);
  /*

  auto newlog = new TimeSeriesProperty<double>(logname);
  newlog->setUnits(oldlog->units());
  int size = oldlog->realSize();
  vector<double> values = oldlog->valuesAsVector();
  vector<DateAndTime> times = oldlog->timesAsVector();
  for (int i = 0; i < size; i++) {
  newlog->addValue(times[i] + offset, values[i]);
  }

  // Just overwrite if the change is in place
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
  IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace");
  duplicate->initialize();
  duplicate->setProperty<Workspace_sptr>(
  "InputWorkspace", boost::dynamic_pointer_cast<Workspace>(inputWS));
  duplicate->execute();
  Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
  outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

  setProperty("OutputWorkspace", outputWS);
  }

  outputWS->mutableRun().addProperty(newlog, true);

    */
}
/* Retrieve and define data search ranges from input workspace properties */
void CalcCountRate::getSearchRanges(
    const DataObjects::EventWorkspace_sptr &InputWorkspace) {
  m_XRangeMin = getProperty("XMin");
  m_XRangeMax = getProperty("XMax");

  if (m_XRangeMin == EMPTY_DBL() && m_XRangeMax == EMPTY_DBL()) {
    m_rangeDefined = false;
  } else {
    m_rangeDefined = true;
  }
}

} // namespace Algorithms
} // namespace Mantid
