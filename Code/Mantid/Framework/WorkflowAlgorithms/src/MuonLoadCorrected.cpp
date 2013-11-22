/*WIKI*
Load Muon data with Dead Time Correction applied. Part of the Muon workflow.
*WIKI*/

#include "MantidWorkflowAlgorithms/MuonLoadCorrected.h"

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MuonLoadCorrected)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MuonLoadCorrected::MuonLoadCorrected()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MuonLoadCorrected::~MuonLoadCorrected()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MuonLoadCorrected::name() const { return "MuonLoadCorrected";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MuonLoadCorrected::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MuonLoadCorrected::category() const { return "Workflow\\Muon";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MuonLoadCorrected::initDocs()
  {
    this->setWikiSummary("Loads Muon data with Dead Time Correction applied.");
    this->setOptionalMessage("Loads Muon data with Dead Time Correction applied.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MuonLoadCorrected::init()
  {
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "The name of the Nexus file to load" );      
    
    std::vector<std::string> dtcTypes;
    dtcTypes.push_back("None");
    dtcTypes.push_back("FromData");
    dtcTypes.push_back("FromSpecifiedFile");

    declareProperty("DtcType","None", boost::make_shared<StringListValidator>(dtcTypes),
      "Type of dead time correction to apply");

    declareProperty(new FileProperty("DtcFile", "", FileProperty::OptionalLoad, ".nxs"),
      "File with dead time values. Used only when DtcType is FromSpecifiedFile.");      

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
      "The name of the workspace to be created as the output of the algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MuonLoadCorrected::exec()
  {
    std::string filename = getPropertyValue("Filename"); 

    IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadMuonNexus");
    loadAlg->setPropertyValue("Filename", filename);
    loadAlg->executeAsChildAlg();

    Workspace_sptr outWS = loadAlg->getProperty("OutputWorkspace");

    setProperty("OutputWorkspace", outWS);
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
