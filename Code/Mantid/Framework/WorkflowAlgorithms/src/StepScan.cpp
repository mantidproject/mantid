#include "MantidWorkflowAlgorithms/StepScan.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace WorkflowAlgorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StepScan)

using namespace Kernel;
using namespace API;

/// Constructor
StepScan::StepScan() {}

/// Destructor
StepScan::~StepScan() {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string StepScan::name() const { return "StepScan"; };

/// Algorithm's version for identification. @see Algorithm::version
int StepScan::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string StepScan::category() const { return "Workflow\\Alignment"; }

void StepScan::init() {
  // TODO: Validator to ensure that this is 'fresh' data???
  declareProperty(new WorkspaceProperty<DataObjects::EventWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "The input workspace. Must hold 'raw' (unweighted) events.");
  // Note that this algorithm may modify the input workspace (by masking and/or
  // cropping)
  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The output table workspace.");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>(
          "MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A workspace holding pixels to be masked.");

  declareProperty("XMin", EMPTY_DBL(),
                  "The minimum value of X for which an event will be counted.");
  declareProperty("XMax", EMPTY_DBL(), "The maximum value of X for which an "
                                       "event will be counted. Must be greater "
                                       "than XMin.");
  // N.B. The choice of units is restricted by the upstream StepScan interface,
  // but in fact any convertible unit will work so is allowed here
  declareProperty("RangeUnit", "TOF", boost::make_shared<StringListValidator>(
                                          UnitFactory::Instance().getKeys()),
                  "The units in which XMin and XMax is being given.");
}

void StepScan::exec() {
  using DataObjects::EventWorkspace_sptr;
  // Get hold of the input workspace
  EventWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get hold of the related monitors workspace, if it exists
  EventWorkspace_sptr monitorWorkspace = getMonitorWorkspace(inputWorkspace);

  // If any of the filtering properties have been set, clone the input workspace
  MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
  const double xmin = getProperty("XMin");
  const double xmax = getProperty("XMax");
  if (maskWS || !isEmpty(xmin) || !isEmpty(xmax)) {
    inputWorkspace = cloneInputWorkspace(inputWorkspace);
  }

  // If the MaskWorkspace property has been set, run the MaskDetectors algorithm
  if (maskWS) {
    runMaskDetectors(inputWorkspace, maskWS);
  }

  // If a restricted X range has been set, handle that
  if (!isEmpty(xmin) || !isEmpty(xmax)) {
    runFilterByXValue(inputWorkspace, xmin, xmax);
    // TODO: In what circumstances should we filter the monitors???
    // if ( monitorWorkspace ) runFilterByXValue(monitorWorkspace, xmin, xmax);
  }

  // Run the SumEventsByLogValue algorithm with the log fixed to 'scan_index'
  IAlgorithm_sptr sumEvents = createChildAlgorithm("SumEventsByLogValue");
  sumEvents->setProperty<EventWorkspace_sptr>("InputWorkspace", inputWorkspace);
  if (monitorWorkspace) {
    sumEvents->setProperty<EventWorkspace_sptr>("MonitorWorkspace",
                                                monitorWorkspace);
  }
  sumEvents->setProperty("LogName", "scan_index");
  sumEvents->executeAsChildAlg();

  Workspace_sptr outputWS = sumEvents->getProperty("OutputWorkspace");
  auto table = boost::dynamic_pointer_cast<ITableWorkspace>(outputWS);
  // Remove the scan_index=0 entry from the resulting table (unless it's the
  // only one)
  if (table->rowCount() > 1 && table->Int(0, 0) == 0) {
    table->removeRow(0);
  }

  setProperty("OutputWorkspace", table);
}

/** Tries to get hold of the workspace that holds the monitor data inside the
 * input workspace.
 *  @param inputWS The input workspace to the algorithm.
 *  @return A pointer to the monitor workspace if found, otherwise a null
 * pointer.
 */
DataObjects::EventWorkspace_sptr
StepScan::getMonitorWorkspace(API::MatrixWorkspace_sptr inputWS) {
  // See if there's a monitor workspace inside the input one
  return boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
      inputWS->monitorWorkspace());
}

DataObjects::EventWorkspace_sptr
StepScan::cloneInputWorkspace(API::Workspace_sptr inputWS) {
  IAlgorithm_sptr clone = createChildAlgorithm("CloneWorkspace");
  clone->setProperty("InputWorkspace", inputWS);
  clone->executeAsChildAlg();

  Workspace_sptr temp = clone->getProperty("OutputWorkspace");
  return boost::static_pointer_cast<DataObjects::EventWorkspace>(temp);
}

/** Runs MaskDetectors as a child algorithm on the input workspace.
 *  @param inputWS The input workspace
 *  @param maskWS  A masking workspace
 */
void StepScan::runMaskDetectors(MatrixWorkspace_sptr inputWS,
                                MatrixWorkspace_sptr maskWS) {
  IAlgorithm_sptr maskingAlg = createChildAlgorithm("MaskDetectors");
  maskingAlg->setProperty<MatrixWorkspace_sptr>("Workspace", inputWS);
  maskingAlg->setProperty<MatrixWorkspace_sptr>("MaskedWorkspace", maskWS);
  maskingAlg->executeAsChildAlg();
}

/** Runs FilterByXValue as a child algorithm on the given workspace
 *  @param inputWS The input workspace
 *  @param xmin    The minimum value of the filter
 *  @param xmax    The maximum value of the filter
 */
void StepScan::runFilterByXValue(MatrixWorkspace_sptr inputWS,
                                 const double xmin, const double xmax) {
  std::string rangeUnit = getProperty("RangeUnit");
  // Run ConvertUnits on the input workspace if xmin/max were given in a
  // different unit
  if (rangeUnit != "TOF") {
    IAlgorithm_sptr convertUnits = createChildAlgorithm("ConvertUnits");
    convertUnits->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    convertUnits->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputWS);
    convertUnits->setProperty("Target", rangeUnit);
    // TODO: Emode/efixed?
    convertUnits->executeAsChildAlg();
  }

  IAlgorithm_sptr filter = createChildAlgorithm("FilterByXValue");
  filter->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
  filter->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputWS);
  filter->setProperty("XMin", xmin);
  filter->setProperty("XMax", xmax);
  filter->executeAsChildAlg();
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
