/*WIKI* 


This algorithm operates similary to calling Plus on two [[EventWorkspace]]s: it combines the events from the two workspaces together to form one large workspace.




*WIKI*/
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/PlusMD.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPool.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PlusMD)
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PlusMD::PlusMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PlusMD::~PlusMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PlusMD::initDocs()
  {
    this->setWikiSummary("Merge two MDEventWorkspaces together by combining their events together in one workspace.");
    this->setOptionalMessage("Merge two MDEventWorkspaces together by combining their events together in one workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PlusMD::init()
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
  void PlusMD::doPlus(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    typename MDEventWorkspace<MDE, nd>::sptr ws1 = ws;
    typename MDEventWorkspace<MDE, nd>::sptr ws2 = boost::dynamic_pointer_cast<MDEventWorkspace<MDE, nd> >(iws2);
    if (!ws1 || !ws2)
      throw std::runtime_error("Incompatible workspace types passed to PlusMD.");

    IMDBox<MDE,nd> * box1 = ws1->getBox();
    IMDBox<MDE,nd> * box2 = ws2->getBox();

    Progress prog(this, 0.0, 0.4, box2->getBoxController()->getTotalNumMDBoxes());

    // Make a leaf-only iterator through all boxes with events in the RHS workspace
    MDBoxIterator<MDE,nd> it2(box2, 1000, true);
    while (true)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(it2.getBox());
      if (box)
      {
        // Copy the events from WS2 and add them into WS1
        const std::vector<MDE> & events = box->getConstEvents();
        // Add events, with bounds checking
        box1->addEvents(events);
        box->releaseEvents();
      }
      prog.report("Adding Events");
      if (!it2.next()) break;
    }

    this->progress(0.41, "Splitting Boxes");
    Progress * prog2 = new Progress(this, 0.4, 0.9, 100);
    ThreadScheduler * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts, 0, prog2);
    ws1->splitAllIfNeeded(ts);
    prog2->resetNumSteps( ts->size(), 0.4, 0.6);
    tp.joinAll();

//    // Now we need to save all the data that was not saved before.
//    if (ws1->isFileBacked())
//    {
//      // Flush anything else in the to-write buffer
//      BoxController_sptr bc = ws1->getBoxController();
//
//      prog.resetNumSteps(bc->getTotalNumMDBoxes(), 0.6, 1.0);
//      MDBoxIterator<MDE,nd> it1(box1, 1000, true);
//      while (true)
//      {
//        MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(it1.getBox());
//        if (box)
//        {
//          // Something was maybe added to this box
//          if (box->getEventVectorSize() > 0)
//          {
//            // By getting the events, this will merge the newly added and the cached events.
//            box->getEvents();
//            // The MRU to-write cache will optimize writes by reducing seek times
//            box->releaseEvents();
//          }
//        }
//        prog.report("Saving");
//        if (!it1.next()) break;
//      }
//      //bc->getDiskMRU().flushCache();
//      // Flush the data writes to disk.
//      box1->flushData();
//    }

    this->progress(0.95, "Refreshing cache");
    ws1->refreshCache();
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PlusMD::exec()
  {
    IMDEventWorkspace_sptr lhs_ws = getProperty("LHSWorkspace");
    IMDEventWorkspace_sptr rhs_ws = getProperty("RHSWorkspace");
    IMDEventWorkspace_sptr out_ws = getProperty("OutputWorkspace");

    if (lhs_ws->id() != rhs_ws->id())
      throw std::invalid_argument("LHS and RHS workspaces must be of the same type and number of dimensions.");

    if ((lhs_ws == out_ws) && (rhs_ws == out_ws))
      throw std::invalid_argument("Sorry, cannot perform PlusMD in place with the same WS on LHS and RHS (A = A + A). Please specify a different output workspace.");

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
      IAlgorithm_sptr clone = this->createSubAlgorithm("CloneMDWorkspace", 0.0, 0.5, true);
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

