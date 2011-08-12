#include "MantidMDEvents/PlusMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PlusMDEW)
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PlusMDEW::PlusMDEW()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PlusMDEW::~PlusMDEW()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PlusMDEW::initDocs()
  {
    this->setWikiSummary("Merge two MDEventWorkspaces together by combining their events together in one workspace.");
    this->setOptionalMessage("Merge two MDEventWorkspaces together by combining their events together in one workspace.");
    this->setWikiDescription(""
        "This algorithm operates similary to calling Plus on two [[EventWorkspace]]s: "
        "it combines the events from the two workspaces together to form one large workspace."
        "\n\n");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PlusMDEW::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("LHSWorkspace","",Direction::Input),
        "One of the workspaces to add together.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("RHSWorkspace","",Direction::Input),
        "One of the workspaces to add together.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "The output workspace. Note that this can be a new workspace, or one of the input workspaces in which case that workspace will be modified in-place.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PlusMDEW::exec()
  {
  }



} // namespace Mantid
} // namespace MDEvents

