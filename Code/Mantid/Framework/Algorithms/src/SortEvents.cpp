//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SortEvents.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(SortEvents)

    /// Sets documentation strings for this algorithm
    void SortEvents::initDocs()
    {
      this->setWikiSummary(" Sort the events in an [[EventWorkspace]], for faster rebinning. ");
      this->setOptionalMessage("Sort the events in an EventWorkspace, for faster rebinning.");
    }


    using namespace Kernel;
    using namespace API;
    using DataObjects::EventList;
    using DataObjects::EventWorkspace;
    using DataObjects::EventWorkspace_sptr;
    using DataObjects::EventWorkspace_const_sptr;

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void SortEvents::init()
    {
      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "",Direction::InOut,new EventWorkspaceValidator<>),
        "EventWorkspace to be sorted.");

      std::vector<std::string> propOptions;
      propOptions.push_back("X Value");
      propOptions.push_back("Pulse Time");
      declareProperty("SortBy", "X Value",new ListValidator(propOptions),
        "How to sort the events:\n"
        "  X Value: the x-position of the event in each pixel (typically Time of Flight).\n"
        "  Pulse Time: the wall-clock time of the pulse that produced the event.");

    }


    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void SortEvents::exec()
    {
      // Get the input workspace
      MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
      //And other properties
      bool sortByTof = (getPropertyValue("SortBy") == "X Value");

      //---------------------------------------------------------------------------------
      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_sptr eventW = boost::dynamic_pointer_cast<EventWorkspace>(inputW);

      if (eventW != NULL)
      {
        //------- EventWorkspace ---------------------------
        const size_t histnumber = inputW->getNumberHistograms();

        //Initialize progress reporting.
        Progress prog(this,0.0,1.0, histnumber);

        DataObjects::EventSortType sortType = DataObjects::TOF_SORT;
        if (!sortByTof) sortType = DataObjects::PULSETIME_SORT;

        //This runs the SortEvents algorithm in parallel
        eventW->sortAll(sortType, &prog);

      } // END ---- EventWorkspace
      else
        throw std::runtime_error("Invalid workspace type provided to SortEvents. Only EventWorkspaces work with this algorithm.");

      return;
    }



  } // namespace Algorithm
} // namespace Mantid
