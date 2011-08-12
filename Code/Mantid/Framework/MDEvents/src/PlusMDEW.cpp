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
    IMDEventWorkspace_sptr lhs_ws = getProperty("LHSWorkspace");
    IMDEventWorkspace_sptr rhs_ws = getProperty("RHSWorkspace");
    IMDEventWorkspace_sptr out_ws = getProperty("OutputWorkspace");

    if (lhs_ws->id() != rhs_ws->id())
      throw std::invalid_argument("LHS and RHS workspaces must be of the same type and number of dimensions.");

    IMDEventWorkspace_sptr ws1;
    IMDEventWorkspace_sptr ws2;

    bool inPlace = false;
    if (out_ws == rhs_ws)
    { // Adding inplace on the right workspace
      inPlace = true;
      ws1 = rhs_ws;
      ws2 = lhs_ws;
    }
    else if (out_ws == lhs_ws)
    { // Adding inplace on the left workspace
      inPlace = true;
      ws1 = lhs_ws;
      ws2 = rhs_ws;
    }
    else
    { // Not adding in place
      inPlace = false;
      // TODO: Clone the lhs workspace into ws1!!!
      ws1 = lhs_ws;
      ws2 = rhs_ws;
      out_ws = ws1;
    }

    // Now we add ws2 into ws1.
    // TODO!
  }



} // namespace Mantid
} // namespace MDEvents

