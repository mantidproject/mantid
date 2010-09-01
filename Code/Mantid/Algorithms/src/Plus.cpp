//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Plus.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Plus)

        //---------------------------------------------------------------------------------------------
    void Plus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),rhsY.begin(),YOut.begin(),std::plus<double>());
      std::transform(lhsE.begin(),lhsE.end(),rhsE.begin(),EOut.begin(),VectorHelper::SumGaussError<double>());
    }

    //---------------------------------------------------------------------------------------------
    void Plus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),YOut.begin(),std::bind2nd(std::plus<double>(),rhsY));
      // Only do E if non-zero, otherwise just copy
      if (rhsE != 0)
        std::transform(lhsE.begin(),lhsE.end(),EOut.begin(),std::bind2nd(VectorHelper::SumGaussError<double>(),rhsE));
      else
        EOut = lhsE;
    }
    
    //---------------------------------------------------------------------------------------------
    bool Plus::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if ( lhs->size() > 1 && rhs->size() > 1 )
      {
        if ( lhs->YUnit() != rhs->YUnit() )
        {
          g_log.error("The two workspaces are not compatible because they have different units for the data (Y).");
          return false;
        }
        if ( lhs->isDistribution() != rhs->isDistribution() )
        {
          g_log.error("The two workspaces are not compatible because one is flagged as a distribution.");
          return false;
        }
      }

      return BinaryOperation::checkCompatibility(lhs,rhs);
    }

    //---------------------------------------------------------------------------------------------
    /** Adds the integrated proton currents, proton charges, of the two input
    *  workspaces together
    *  @param lhs one of the workspace samples to be summed
    *  @param rhs the other workspace sample to be summed
    *  @param ans the sample in the output workspace
    */
    void Plus::operateOnRun(const Run& lhs, const Run& rhs, Run& ans) const
    {
      try
      {
        ans.setProtonCharge( lhs.getProtonCharge() + rhs.getProtonCharge() );
      }
      catch(Exception::NotFoundError &)
      {
      }
    }


    //---------------------------------------------------------------------------------------------
    /** Return true if both workspaces are event workspaces, and they both match in instrument name
     * and number of spectra.
     * @throw std::invalid_argument if they are event workspaces but can't be added
     */
    bool Plus::checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs)
    {
      //Get the names of the workspaces
      lhs_name = getPropertyValue(inputPropName1());
      rhs_name = getPropertyValue(inputPropName2());
      out_name = getPropertyValue(outputPropName());
      //If names match, we will be adding in place
      bool addingInPlace = (lhs_name == out_name) || (rhs_name == out_name);

      EventWorkspace_const_sptr event_lhs = boost::dynamic_pointer_cast<const EventWorkspace>(lhs);
      EventWorkspace_const_sptr event_rhs = boost::dynamic_pointer_cast<const EventWorkspace>(rhs);

      //Both have to be valid event workspaces
      if (!event_lhs || !event_rhs)
        return false;

      //Mismatched instruments?
      if (event_lhs->getInstrument() && event_rhs->getInstrument())
        if (event_lhs->getInstrument()->getName() != event_rhs->getInstrument()->getName())
        {
          throw std::invalid_argument("Cannot add event workspaces; mismatched instrument names.");
        }

      //Must match in number of histograms if adding in place
      if (addingInPlace)
      {
        if ((event_lhs->getNumberHistograms() != event_rhs->getNumberHistograms()))
        {
          throw std::invalid_argument("Cannot add event workspaces in place together if the number of histograms does not match. Please specify a different output workspace name for the addition; or Pad Pixels when loading neutron event files.");
        }
        else
        {
          //Matching # of histograms, but that doesn't mean the detector IDs match too!
          //TODO: Check that the full detector list matches
          IndexToIndexMap * lhs_map = lhs->getWorkspaceIndexToDetectorIDMap();
          IndexToIndexMap * rhs_map = rhs->getWorkspaceIndexToDetectorIDMap();
          for (int i=0; i < lhs->getNumberHistograms(); i++)
            if ((*lhs_map)[i] != (*rhs_map)[i])
            {
              delete lhs_map;
              delete rhs_map;
              throw std::invalid_argument("Cannot add event workspaces in place together because the pixel IDs are not the same at all workspace indices.");
            }

          delete lhs_map;
          delete rhs_map;

        }
      }


      //All is good if we got here
      return true;
    }

//    /** */
//    void Plus::buildAdditionMap()
//    {
//
//    }

    //---------------------------------------------------------------------------------------------
    /** Perform the plus operation on the two event workspaces. This will only be called if they are compatible.
     *
     */
    void Plus::execEvent( DataObjects::EventWorkspace_const_sptr lhs, DataObjects::EventWorkspace_const_sptr rhs )
    {
      //std::cout << "Running on events!\n";

      //Get the output workspace
      EventWorkspace_sptr outWS;
      MatrixWorkspace_sptr outMatrixWS = getProperty(outputPropName());
      outWS = boost::dynamic_pointer_cast<EventWorkspace>(outMatrixWS);

      //The workspace that will be added to the other
      EventWorkspace_const_sptr adder;

      //Progress reporting
      Progress progress(this,0.0,1.0, lhs->getNumberHistograms()+rhs->getNumberHistograms() );

      //Are you adding workspaces in place?
      if ((outWS == lhs) || (outWS == rhs))
      {
        if (outWS == lhs)
        {
          adder = rhs;
        }
        else if (outWS == rhs)
        {
          adder = lhs;
        }

        //Go through all the histograms and add up the histograms
        //(Number of histograms has to match, from checkEventCompatibility)
        int numhist = outWS->getNumberHistograms();
        for (int i=0; i < numhist; i++)
        {
          //Concatenate event lists
          outWS->getEventListAtWorkspaceIndex(i) += adder->getEventListAtWorkspaceIndex(i);
          //Two steps (handled two lists at once)
          progress.report(2);
        }
        //Clear the MRU list since its not valid anymore.
        outWS->clearMRU();

      } //------------ adding in place ----------------


      else if (!outWS)
      {
        //outWS is null since it is a new workspace

        //Get the required maps for accessing the detector IDs
        IndexToIndexMap * lhs_map = lhs->getWorkspaceIndexToDetectorIDMap();
        IndexToIndexMap * rhs_map = rhs->getWorkspaceIndexToDetectorIDMap();

        //Create a copy of the lhs workspace
        outWS = boost::dynamic_pointer_cast<EventWorkspace>(API::WorkspaceFactory::Instance().create("EventWorkspace", lhs->getNumberHistograms(), 2, 1));
        //Copy geometry, spectra map, etc. over.
        API::WorkspaceFactory::Instance().initializeFromParent(lhs, outWS, false);
        //But we don't copy any data yet.

        //Go through all the histograms of lhs
        int numhist, pid;
        numhist = lhs->getNumberHistograms();
        for (int i=0; i < numhist; i++)
        {
          //Pixel ID of that histogram #
          pid = (*lhs_map)[i];
          //Concatenate event lists
          outWS->getEventList(pid) += lhs->getEventListAtWorkspaceIndex(i);
          //Copy the cow_ptr to the same X axis used in the other
          outWS->getEventList(pid).setX( lhs->refX(i) );

          progress.report();
        }

        //Same for the RHS workspace
        numhist = rhs->getNumberHistograms();
        for (int i=0; i < numhist; i++)
        {
          pid = (*rhs_map)[i];
          outWS->getEventList(pid) += rhs->getEventListAtWorkspaceIndex(i);
          outWS->getEventList(pid).setX( rhs->refX(i) );
          progress.report();
        }

        //Finalize the event list histograms and such.
        outWS->doneLoadingData();

        delete lhs_map;
        delete rhs_map;
      }

      else
      {
        //Should never happen
        throw Exception::NotImplementedError("Plus::execEvent:outWS came out non-null, but does not match either lhs or rhs workspaces. This should not happen.");
      }

      // only overridden for some operations (plus and minus at the time of writing)
      operateOnRun(lhs->run(), rhs->run(), outWS->mutableRun());

      // Assign the result to the output workspace property
      setProperty(outputPropName(), boost::dynamic_pointer_cast<MatrixWorkspace>(outWS));

    }


  }
}
