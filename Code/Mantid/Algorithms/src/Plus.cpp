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
      (void) lhsX; //Avoid compiler warning
      std::transform(lhsY.begin(),lhsY.end(),rhsY.begin(),YOut.begin(),std::plus<double>());
      std::transform(lhsE.begin(),lhsE.end(),rhsE.begin(),EOut.begin(),VectorHelper::SumGaussError<double>());
    }

    //---------------------------------------------------------------------------------------------
    void Plus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      std::transform(lhsY.begin(),lhsY.end(),YOut.begin(),std::bind2nd(std::plus<double>(),rhsY));
      // Only do E if non-zero, otherwise just copy
      if (rhsE != 0)
        std::transform(lhsE.begin(),lhsE.end(),EOut.begin(),std::bind2nd(VectorHelper::SumGaussError<double>(),rhsE));
      else
        EOut = lhsE;
    }
    
    //---------------------------------------------------------------------------------------------
    /* Return true if the units and distribution-type of the workspaces make them compatible */
    bool Plus::checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
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
      return true;
    }

    //---------------------------------------------------------------------------------------------
    bool Plus::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if (!checkUnitCompatibility(lhs, rhs))
        return false;

      //Keep checking more generally.
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
      //The addition operator of Run will add the proton charges, append logs, etc.
      ans = lhs;
      ans += rhs;
    }


    //---------------------------------------------------------------------------------------------
    /** Return true if both workspaces are event workspaces, and they both match in instrument name
     * and number of spectra.
     * @return  true if they are both event workspaces and everything else is valid
     *          false if they are not event workspaces.
     * @throw std::invalid_argument if they are event workspaces, but can't be added for some other reason
     */
    bool Plus::checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs)
    {
      //Get the names of the workspaces
      lhs_name = getPropertyValue(inputPropName1());
      rhs_name = getPropertyValue(inputPropName2());
      out_name = getPropertyValue(outputPropName());

      EventWorkspace_const_sptr event_lhs = boost::dynamic_pointer_cast<const EventWorkspace>(lhs);
      EventWorkspace_const_sptr event_rhs = boost::dynamic_pointer_cast<const EventWorkspace>(rhs);

      //Both have to be valid event workspaces
      if (!event_lhs || !event_rhs)
        return false;

      //Matching units?
      if (!checkUnitCompatibility(lhs, rhs))
        throw std::invalid_argument("Cannot add event workspaces; mismatched Y units.");

      //Mismatched instruments?
      if (event_lhs->getInstrument() && event_rhs->getInstrument())
        if (event_lhs->getInstrument()->getName() != event_rhs->getInstrument()->getName())
        {
          throw std::invalid_argument("Cannot add event workspaces; mismatched instrument names.");
        }

      //Must match in number of histograms
      if ((event_lhs->getNumberHistograms() != event_rhs->getNumberHistograms()))
      {
        throw std::invalid_argument("Cannot add event workspaces together if the number of histograms does not match. Please use MergeRuns to merge runs with different pixel lists; or Pad Pixels when loading neutron event files.");
      }


      //All is good if we got here
      return true;
    }

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
      }
      else if (!outWS)
      {
        //outWS is null since it is a new workspace
        //Create a copy of the lhs workspace
        outWS = boost::dynamic_pointer_cast<EventWorkspace>(API::WorkspaceFactory::Instance().create("EventWorkspace", lhs->getNumberHistograms(), 2, 1));
        //Copy geometry, spectra map, etc. over.
        API::WorkspaceFactory::Instance().initializeFromParent(lhs, outWS, false);
        //And we copy all the events from the lhs
        outWS->copyDataFrom( *lhs );
        //And this is what we'll add in to it
        adder = rhs;
      }
      else
      {
        //Should never happen
        throw Exception::NotImplementedError("Plus::execEvent:outWS came out non-null, but does not match either lhs or rhs workspaces. This should not happen.");
      }

      //Go through all the histograms and add up the event lists
      //(Number of histograms has to match, from checkEventCompatibility)
      int numhist = outWS->getNumberHistograms();
      for (int i=0; i < numhist; i++)
      {
        //Concatenate event lists
        outWS->getEventList(i) += adder->getEventList(i);
        //Two steps (handled two lists at once)
        progress.report(2);
      }
      //Redo the spectra to detector IDs map
      outWS->makeSpectraMap();
      //Clear the MRU list since its not valid anymore.
      outWS->clearMRU();

      // only overridden for some operations (plus and minus at the time of writing)
      operateOnRun(lhs->run(), rhs->run(), outWS->mutableRun());

      // Assign the result to the output workspace property
      setProperty(outputPropName(), boost::dynamic_pointer_cast<MatrixWorkspace>(outWS));

    }


  }
}
