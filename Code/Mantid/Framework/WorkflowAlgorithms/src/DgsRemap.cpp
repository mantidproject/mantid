/*WIKI*

This algorithm is responsible for masking and grouping the given input workspace.
One can use the ExecuteOppositeOrder to do grouping first then masking.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsRemap.h"
#include "MantidDataObjects/GroupingWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsRemap)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsRemap::DgsRemap()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsRemap::~DgsRemap()
  {
  }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsRemap::name() const { return "DgsRemap"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsRemap::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsRemap::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsRemap::initDocs()
  {
    this->setWikiSummary("Mask and/or group the given workspace.");
    this->setOptionalMessage("Mask and/or group the given workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsRemap::init()
  {
    this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace",
        "", Direction::Input), "An input workspace to mask and group.");
    this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("MaskWorkspace",
        "", Direction::Input, PropertyMode::Optional),
        "A workspace containing masking information.");
    this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("GroupingWorkspace",
        "", Direction::Input, PropertyMode::Optional),
        "A workspace containing grouping information");
    this->declareProperty("ExecuteOppositeOrder", false,
        "Execute grouping before masking.");
    this->declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace",
        "", Direction::Output), "The resulting workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsRemap::exec()
  {
    MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
    MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");

    bool runOpposite = this->getProperty("ExecuteOppositeOrder");
    if (!runOpposite)
    {
      this->execMasking(inputWS);
      this->execGrouping(inputWS, outputWS);
    }
    else
    {
      this->execGrouping(inputWS, outputWS);
      this->execMasking(inputWS);
    }

    this->setProperty("OutputWorkspace", outputWS);
  }

  void DgsRemap::execMasking(MatrixWorkspace_sptr iWS)
  {
    MatrixWorkspace_sptr maskWS = this->getProperty("MaskWorkspace");
    if (maskWS)
    {
      IAlgorithm_sptr mask = this->createSubAlgorithm("MaskDetectors");
      mask->setProperty("Workspace", iWS);
      mask->setProperty("MaskedWorkspace", maskWS);
      mask->executeAsSubAlg();
    }
  }

  void DgsRemap::execGrouping(MatrixWorkspace_sptr iWS, MatrixWorkspace_sptr &oWS)
  {
    MatrixWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
    if (groupWS)
    {
      int64_t ngroups = 0;
      std::vector<int> groupDetIdList;
      GroupingWorkspace_sptr gWS = boost::dynamic_pointer_cast<GroupingWorkspace>(groupWS);
      gWS->makeDetectorIDToGroupVector(groupDetIdList, ngroups);

      IAlgorithm_sptr group = this->createSubAlgorithm("GroupDetectors");
      group->setProperty("InputWorkspace", iWS);
      group->setProperty("OutputWorkspace", iWS);
      group->setProperty("DetectorList", groupDetIdList);
      group->setProperty("Behaviour", "Average");
      group->executeAsSubAlg();
      oWS = group->getProperty("OutputWorkspace");
    }
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
