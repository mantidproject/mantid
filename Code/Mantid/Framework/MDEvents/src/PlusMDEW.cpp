#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/PlusMDEW.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPool.h"

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
  /** Perform the adding
   *
   * @param ws ::  MDEventWorkspace to clone
   */
  template<typename MDE, size_t nd>
  void PlusMDEW::doPlus(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    typename MDEventWorkspace<MDE, nd>::sptr ws1 = ws;
    typename MDEventWorkspace<MDE, nd>::sptr ws2 = boost::dynamic_pointer_cast<MDEventWorkspace<MDE, nd> >(iws2);
    if (!ws1 || !ws2)
      throw std::runtime_error("Incompatible workspace types passed to PlusMDEW.");

    IMDBox<MDE,nd> * box1 = ws1->getBox();
    IMDBox<MDE,nd> * box2 = ws2->getBox();

    Progress prog(this, 0.0, 0.9, box2->getBoxController()->getTotalNumMDBoxes());

    // Make a leaf-only iterator through all boxes with events in the workspace
    MDBoxIterator<MDE,nd> it(box2, 1000, true);
    while (true)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(it.getBox());
      if (box)
      {
        // Copy the events from WS2 and add them into WS1
        const std::vector<MDE> & events = box->getConstEvents();
        // Add events, with bounds checking
        box1->addEvents(events);
        box->releaseEvents();
      }
      prog.report();
      if (!it.next()) break;
    }

    prog.resetNumSteps(3, 0.9, 1.0);
    prog.report("Splitting Boxes");
    ThreadScheduler * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);
    ws1->splitAllIfNeeded(ts);
    tp.joinAll();

    prog.report("Refreshing cache");
    ws1->refreshCache();
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

    if ((lhs_ws == out_ws) && (rhs_ws == out_ws))
      throw std::invalid_argument("Sorry, cannot perform PlusMDEW in place with the same WS on LHS and RHS (A = A + A). Please specify a different output workspace.");

    bool inPlace = false;
    if (out_ws == rhs_ws)
    { // Adding inplace on the right workspace
      inPlace = true;
      iws1 = rhs_ws;
      iws2 = lhs_ws;
    }
    else if (out_ws == lhs_ws)
    { // Adding inplace on the left workspace
      inPlace = true;
      iws1 = lhs_ws;
      iws2 = rhs_ws;
    }
    else
    { // Not adding in place
      inPlace = false;

      // If you have to clone any WS, clone the one that is file-backed.
      bool cloneLHS = true;
      if (rhs_ws->isFileBacked() && !lhs_ws->isFileBacked())
        cloneLHS = false;

      // Clone the lhs workspace into ws1
      IAlgorithm_sptr clone = this->createSubAlgorithm("CloneMDEventWorkspace", 0.0, 0.5, true);
      clone->setProperty("InputWorkspace", (cloneLHS ? lhs_ws : rhs_ws) );
      clone->setPropertyValue("OutputWorkspace", getPropertyValue("OutputWorkspace"));
      clone->executeAsSubAlg();
      iws1 = clone->getProperty("OutputWorkspace");

      iws2 = (cloneLHS ? rhs_ws : lhs_ws); // The other one (not cloned) goes on the RHS
      out_ws = iws1;
    }

    // Now we add ws2 into ws1.
    CALL_MDEVENT_FUNCTION(this->doPlus, iws1);

    // Set to the output
    setProperty("OutputWorkspace", out_ws);
  }



} // namespace Mantid
} // namespace MDEvents

