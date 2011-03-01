//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Sort.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Sort)

    using namespace Kernel;
    using namespace API;
    using DataObjects::EventList;
    using DataObjects::EventWorkspace;
    using DataObjects::EventWorkspace_sptr;
    using DataObjects::EventWorkspace_const_sptr;

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void Sort::init()
    {
      //this->setWikiSummary("Sort the events in an [[EventWorkspace]], for faster rebinning.");
      //this->setOptionalMessage("Sort the events in an EventWorkspace, for faster rebinning.");

      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "",Direction::InOut,new EventWorkspaceValidator<>),
        "EventWorkspace to be sorted.");

      std::vector<std::string> propOptions;
      propOptions.push_back("Time of Flight");
      propOptions.push_back("Pulse Time");
      declareProperty("SortBy", "Time of Flight",new ListValidator(propOptions),
        "How to sort the events.");

    }


    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void Sort::exec()
    {
      // Get the input workspace
      MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
      //And other properties
      bool sortByTof = (getPropertyValue("SortBy") == "Time of Flight");

      //---------------------------------------------------------------------------------
      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_sptr eventW = boost::dynamic_pointer_cast<EventWorkspace>(inputW);

      if (eventW != NULL)
      {
        //------- EventWorkspace ---------------------------
        const int histnumber = inputW->getNumberHistograms();

        //Initialize progress reporting.
        Progress prog(this,0.0,1.0, histnumber);

        DataObjects::EventSortType sortType = DataObjects::TOF_SORT;
        if (!sortByTof) sortType = DataObjects::PULSETIME_SORT;

        //This runs the sort algorithm in parallel
        eventW->sortAll(sortType, &prog);

      } // END ---- EventWorkspace
      else
        throw std::runtime_error("Invalid workspace type provided to Sort. Only EventWorkspaces work with this algorithm.");

      return;
    }



  } // namespace Algorithm
} // namespace Mantid
