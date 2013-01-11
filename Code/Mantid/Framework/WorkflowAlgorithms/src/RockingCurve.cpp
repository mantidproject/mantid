/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidWorkflowAlgorithms/RockingCurve.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RockingCurve)
  
  using namespace Kernel;
  using namespace API;

  /// Constructor
  RockingCurve::RockingCurve()
  {}

  /// Destructor
  RockingCurve::~RockingCurve()
  {}

  /// Algorithm's name for identification. @see Algorithm::name
  const std::string RockingCurve::name() const { return "RockingCurve";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int RockingCurve::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string RockingCurve::category() const { return "Workflow\\Alignment";}

  void RockingCurve::initDocs()
  {
    this->setWikiSummary("Workflow algorithm for analysis of an alignment scan.");
    this->setOptionalMessage("Workflow algorithm for analysis of an alignment scan.");
  }

  void RockingCurve::init()
  {
    // TODO: Validator to ensure that this is 'fresh' data???
    declareProperty(new WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input,
        boost::make_shared<WorkspaceUnitValidator>("TOF")), "An input workspace.");
    // Note that this algorithm may modify the input workspace (by masking and/or cropping)
    declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("MaskWorkspace","",Direction::Input, PropertyMode::Optional), "");

    declareProperty("XMin", EMPTY_DBL());
    declareProperty("XMax", EMPTY_DBL());
    // TODO: Restrict the choice of units?
    declareProperty("RangeUnit", "TOF", boost::make_shared<StringListValidator>(UnitFactory::Instance().getKeys()),
      "The units of XMin and XMax" );

    // TODO: Maybe need to add a pre/post-processing flag for live
  }

  void RockingCurve::exec()
  {
    // Get hold of the input workspace
    DataObjects::EventWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
    // Get hold of the related monitors workspace, if it exists
    // TODO: How will this work for live data???
    DataObjects::EventWorkspace_sptr monitorWorkspace = getMonitorWorkspace(inputWorkspace);

    // If the MaskWorkspace property has been set, run the MaskDetectors algorithm
    MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
    if ( maskWS )
    {
      runMaskDetectors(inputWorkspace, maskWS);
    }

    // If a restricted X range has been set, handle that
    const double xmin = getProperty("XMin");
    const double xmax = getProperty("XMax");
    if ( !isEmpty(xmin) || !isEmpty(xmax) )
    {
      runFilterByXValue(inputWorkspace, xmin, xmax);
      // TODO: In what circumstances should we filter the monitors???
      //if ( monitorWorkspace ) runFilterByXValue(monitorWorkspace, xmin, xmax);
    }

    // Run the SumEventsByLogValue algorithm with the log fixed to 'scan_index'
    IAlgorithm_sptr sumEvents = createChildAlgorithm("SumEventsByLogValue");
    sumEvents->setProperty<DataObjects::EventWorkspace_sptr>("InputWorkspace", inputWorkspace);
    sumEvents->setProperty("LogName", "scan_index");
    sumEvents->executeAsChildAlg();

    Workspace_sptr outputWS = sumEvents->getProperty("OutputWorkspace");
    auto table = boost::dynamic_pointer_cast<ITableWorkspace>(outputWS);
    // Remove the scan_index=0 entry from the resulting table (will be the first row)
    table->removeRow(0);

    setProperty("OutputWorkspace",table);
  }

  /** Tries to get hold of the workspace that holds the monitor data for the input workspace.
   *  Does this by looking for a workspace with the same name as the input with "_monitors" appended.
   *  @param inputWS The input workspace to the algorithm.
   *  @return A pointer to the monitor workspace if found, otherwise a null pointer.
   */
  DataObjects::EventWorkspace_sptr RockingCurve::getMonitorWorkspace(API::MatrixWorkspace_sptr inputWS)
  {
    // See if there's a monitor workspace alongside the input one
    const std::string monitorWorkspaceName = inputWS->name() + "_monitors";
    DataObjects::EventWorkspace_sptr monitorWorkspace;
    try {
      monitorWorkspace = AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>(monitorWorkspaceName);
      // Check that we have an EventWorkspace for the monitors. If not, just return.
      if ( !monitorWorkspace )
      {
        g_log.warning() << "A monitor workspace (" << monitorWorkspaceName << ") was found, but "
            << "it is not an EventWorkspace so cannot be used in this algorithm.\n";
      }
    } catch (Exception::NotFoundError&) {
      // The monitors workspace isn't there - just return
      g_log.information() << "No monitor workspace (" << monitorWorkspaceName << ") found.\n";
    }

    return monitorWorkspace;
  }

  /** Runs MaskDetectors as a child algorithm on the input workspace.
   *  @param inputWS The input workspace
   *  @param maskWS  A masking workspace
   */
  void RockingCurve::runMaskDetectors(MatrixWorkspace_sptr inputWS, MatrixWorkspace_sptr maskWS)
  {
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
  void RockingCurve::runFilterByXValue(MatrixWorkspace_sptr inputWS, const double xmin, const double xmax)
  {
    std::string rangeUnit = getProperty("RangeUnit");
    // Run ConvertUnits on the input workspace if xmin/max were given in a different unit
    if ( rangeUnit != "TOF" )
    {
      IAlgorithm_sptr convertUnits = createChildAlgorithm("ConvertUnits");
      convertUnits->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
      convertUnits->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputWS);
      convertUnits->setProperty("Target", rangeUnit);
      // Emode/efixed?
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
