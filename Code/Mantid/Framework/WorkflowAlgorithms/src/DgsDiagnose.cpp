/*WIKI*

This algorithm is responsible for setting up the necessary workspaces to
hand off to the DetectorDiagnostic algorithm.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsDiagnose.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsDiagnose)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsDiagnose::DgsDiagnose()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsDiagnose::~DgsDiagnose()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsDiagnose::name() const { return "DgsDiagnose"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsDiagnose::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsDiagnose::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsDiagnose::initDocs()
  {
    this->setWikiSummary("Setup and run DetectorDiagnostic.");
    this->setOptionalMessage("Setup and run DetectorDiagnostic.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsDiagnose::init()
  {
    this->declareProperty(new WorkspaceProperty<>("DetVanWorkspace", "",
        Direction::Input), "The detector vanadium workspace.");
    this->declareProperty(new WorkspaceProperty<>("DetVanCompWorkspace", "",
        Direction::Input),
        "A detector vanadium workspace to compare against the primary one.");
    this->declareProperty(new WorkspaceProperty<>("SampleWorkspace", "",
        Direction::Input, PropertyMode::Optional),
        "A sample workspace to run some diagnostics on.");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "",
        Direction::Output), "An output workspace.");
    this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
        Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsDiagnose::exec()
  {
    g_log.notice() << "Starting DgsDiagnose" << std::endl;
    // Get the reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    boost::shared_ptr<PropertyManager> reductionManager;
    if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
      {
        reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
      }
    else
      {
        throw std::runtime_error("DgsDiagnose cannot run without a reduction PropertyManager.");
      }

    this->enableHistoryRecordingForChild(true);

  }



} // namespace WorkflowAlgorithms
} // namespace Mantid
