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
      this->g_log.setName("Algorithms::Sort");

      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "",Direction::InOut,new EventWorkspaceValidator<>),
        "Workspace to be sorted.");

      //TODO: Invent a "choice" property and put it here

      //Sort by TOF, defaults to TRUE
      declareProperty(
        new PropertyWithValue<bool>("SortByTOF", true),
        "Check to sort by TOF; otherwise, will sort by frame index.");
    }


    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void Sort::exec()
    {
      // Get the input workspace
      MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");
      //And other properties
      bool sortByTof = getProperty("SortByTOF");

      //---------------------------------------------------------------------------------
      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputW);

      if (eventW != NULL)
      {
        //------- EventWorkspace ---------------------------
        const int histnumber = inputW->getNumberHistograms();

        //Initialize progress reporting.
        Progress prog(this,0.0,1.0, histnumber);

        //Go through all the histograms and set the data
        PARALLEL_FOR1(eventW)
        for (int i=0; i < histnumber; ++i)
        {
          PARALLEL_START_INTERUPT_REGION

          //Perform the sort
          if (sortByTof)
            eventW->getEventListAtWorkspaceIndex(i).sortTof();
          else
            eventW->getEventListAtWorkspaceIndex(i).sortFrame();

          //Report progress
          prog.report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION

      } // END ---- EventWorkspace
      else
        throw std::runtime_error("Invalid workspace type provided to Sort. Only EventWorkspaces work with this algorithm.");

      return;
    }



  } // namespace Algorithm
} // namespace Mantid
