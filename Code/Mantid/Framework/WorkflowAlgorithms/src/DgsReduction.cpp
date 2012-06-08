/*WIKI*

This is the top-level workflow algorithm for direct geometry spectrometer
data reduction. This algorithm is responsible for gathering the necessary
parameters and generating calls to other workflow or standard algorithms.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsReduction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsReduction)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsReduction::DgsReduction()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsReduction::~DgsReduction()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsReduction::name() const { return "DgsReduction"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsReduction::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsReduction::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsReduction::initDocs()
  {
    this->setWikiSummary("Top-level workflow algorithm for DGS reduction.");
    this->setOptionalMessage("Top-level workflow algorithm for DGS reduction.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsReduction::init()
  {
    //declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    //declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
    declareProperty("SampleData", "", "Run numbers, files or workspaces of the data sets to be reduced");
    declareProperty("FilterBadPulses", false, "If true, filter bad pulses from data.");
    std::vector<std::string> incidentBeamNormOptions;
    incidentBeamNormOptions.push_back("None");
    incidentBeamNormOptions.push_back("ByCurrent");
    incidentBeamNormOptions.push_back("ToMonitor");
    declareProperty("IncidentBeamNormalisation", "None",
        boost::make_shared<StringListValidator>(incidentBeamNormOptions),
        "Options for incident beam normalisation on data.");
    declareProperty("TimeIndepBackgroundRemoval", false,
        "If true, time-independent background will be calculated and removed.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsReduction::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace WorkflowAlgorithms
