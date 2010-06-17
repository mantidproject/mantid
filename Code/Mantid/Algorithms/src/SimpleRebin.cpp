//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleRebin.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
//#include <boost/cast.hpp>


namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(SimpleRebin)

    using namespace Kernel;
    using namespace API;
    using DataObjects::Workspace2D;
    using DataObjects::Workspace2D_sptr;
    using DataObjects::EventWorkspace;
    using DataObjects::EventWorkspace_sptr;
    using DataObjects::EventWorkspace_const_sptr;

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void SimpleRebin::init()
    {
      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "",Direction::InOut,new HistogramValidator<>),
        "Workspace containing the input data");
      declareProperty(
        new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "The name to give the output workspace");

      declareProperty(
        new ArrayProperty<double>("Params", new RebinParamsValidator),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be folowed by a comma and more widths and last boundary pairs");
    }

    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void SimpleRebin::exec()
    {
      // Get the input workspace
      MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

      // retrieve the properties
      std::vector<double> rb_params=getProperty("Params");

      bool dist = inputW->isDistribution();

      // workspace independent determination of length
      const int histnumber = inputW->getNumberHistograms();
      DataObjects::Histogram1D::RCtype XValues_new;
      // create new output X axis
      const int ntcnew = VectorHelper::createAxisFromRebinParams(rb_params,XValues_new.access());

      //---------------------------------------------------------------------------------
      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_sptr eventW = boost::dynamic_pointer_cast<EventWorkspace>(inputW);

      if (eventW != NULL)
      {
        //------- EventWorkspace ---------------------------
        EventWorkspace_sptr eventOutW;
        if (getPropertyValue("OutputWorkspace") == getPropertyValue("InputWorkspace"))
        {
          //Same output as input; don't copy data for no reason.
          eventOutW = eventW;
        }
        else
        {
          //Need to copy over a new workspace
          //std::cout << "Creating copy \n";
          eventOutW = eventW;
          //eventOutW =  boost::dynamic_pointer_cast<EventWorkspace>( API::WorkspaceFactory::Instance().create(eventW,histnumber,ntcnew,ntcnew-1) );
        }
        //This only sets the X axis. Actual rebinning will be done upon data access.
        //std::cout << "setAllX\n";
        eventOutW->setAllX(XValues_new);

        //Copy the units over too.
        //std::cout << "getAxis\n";
        for (int i=0; i < eventOutW->axes(); ++i)
        {
          eventOutW->getAxis(i)->unit() = inputW->getAxis(i)->unit();
        }

        //std::cout << "setProperty(OutputWorkspace\n";
        // Assign it to the output workspace property; recasting to matrixworkspace
        setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(eventOutW) );

        //std::cout << "done\n";
      } // END ---- EventWorkspace

      else

      { //------- Workspace2D or other MatrixWorkspace ---------------------------

        // make output Workspace the same type is the input, but with new length of signal array
        API::MatrixWorkspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,ntcnew,ntcnew-1);


        // Copy over the 'vertical' axis
        if (inputW->axes() > 1) outputW->replaceAxis( 1, inputW->getAxis(1)->clone(outputW.get()) );

        Progress prog(this,0.0,1.0,histnumber);
        PARALLEL_FOR2(inputW,outputW)
        for (int hist=0; hist <  histnumber;++hist)
        {
          PARALLEL_START_INTERUPT_REGION
          // get const references to input Workspace arrays (no copying)
          const MantidVec& XValues = inputW->readX(hist);
          const MantidVec& YValues = inputW->readY(hist);
          const MantidVec& YErrors = inputW->readE(hist);

          //get references to output workspace data (no copying)
          MantidVec& YValues_new=outputW->dataY(hist);
          MantidVec& YErrors_new=outputW->dataE(hist);

          // output data arrays are implicitly filled by function
          try {
            VectorHelper::rebin(XValues,YValues,YErrors,*XValues_new,YValues_new,YErrors_new, dist);
          } catch (std::exception& ex)
          {
            g_log.error() << "Error in rebin function: " << ex.what() << std::endl;
            throw ex;
          }

          // Populate the output workspace X values
          outputW->setX(hist,XValues_new);
          ////copy oer the spectrum No and ErrorHelper
          //try {
          //  outputW->getAxis(1)->setValue(hist,(*(inputW->getAxis(1)))(hist));
          //  //outputW->getAxis(1)->spectraNo(hist)=inputW->getAxis(1)->spectraNo(hist);
          //} catch (Exception::IndexError) {
          //  // OK, so this isn't a Workspace2D
          //}

          prog.report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
        outputW->isDistribution(dist);

        // Now propagate any masking correctly to the output workspace
        // More efficient to have this in a separate loop because
        // MatrixWorkspace::maskBins blocks multi-threading
        for (int i=0; i <  histnumber; ++i)
        {
          if ( inputW->hasMaskedBins(i) )  // Does the current spectrum have any masked bins?
          {
            this->propagateMasks(inputW,outputW,i);
          }
        }
        //Copy the units over too.
        for (int i=0; i < outputW->axes(); ++i)
        {
          outputW->getAxis(i)->unit() = inputW->getAxis(i)->unit();
        }

        // Assign it to the output workspace property
        setProperty("OutputWorkspace",outputW);


      } // END ---- Workspace2D

      return;
    }

//
//    /** Continue execution for EventWorkspace scenario */
//    void SimpleRebin::execEvent()
//    {
//      // retrieve the properties
//      std::vector<double> rb_params=getProperty("Params");
//
//    }

    /** Takes the masks in the input workspace and apportions the weights into the new bins that overlap
     *  with a masked bin. These bins are then masked with the calculated weight.
     * 
     *  @param inputW  The input workspace
     *  @param outputW The output workspace
     *  @param hist    The index of the current histogram
     */
    void SimpleRebin::propagateMasks(API::MatrixWorkspace_const_sptr inputW, API::MatrixWorkspace_sptr outputW, int hist)
    {
      // Not too happy with the efficiency of this way of doing it, but it's a lot simpler to use the
      // existing rebin algorithm to distribute the weights than to re-implement it for this
      
      MantidVec masked_bins,weights;
      // Get a reference to the list of masked bins for this spectrum
      const MatrixWorkspace::MaskList& mask = inputW->maskedBins(hist);
      // Now iterate over the list, building up a vector of the masked bins
      MatrixWorkspace::MaskList::const_iterator it = mask.begin();
      const MantidVec& XValues = inputW->readX(hist);
      masked_bins.push_back(XValues[(*it).first]);
      weights.push_back((*it).second);
      masked_bins.push_back(XValues[(*it).first + 1]);
      for (++it; it!= mask.end(); ++it)
      {
        const double currentX = XValues[(*it).first];
        // Add an intermediate bin with zero weight if masked bins aren't consecutive
        if (masked_bins.back() != currentX) 
        {
          weights.push_back(0.0);
          masked_bins.push_back(currentX);
        }
        weights.push_back((*it).second);
        masked_bins.push_back(XValues[(*it).first + 1]);
      }
      
      // Create a zero vector for the errors because we don't care about them here
      const MantidVec zeroes(weights.size(),0.0);
      // Create a vector to hold the redistributed weights
      const MantidVec& XValues_new = outputW->readX(hist);
      MantidVec newWeights(XValues_new.size()-1),zeroes2(XValues_new.size()-1);
      // Use rebin function to redistribute the weights. Note that distribution flag is set
      VectorHelper::rebin(masked_bins,weights,zeroes,XValues_new,newWeights,zeroes2,true);
      
      // Now process the output vector and fill the new masking list
      for (size_t index = 0; index < newWeights.size(); ++index)
      {
        if ( newWeights[index] > 0.0 ) outputW->maskBin(hist,index,newWeights[index]);
      }
    }
    
  } // namespace Algorithm
} // namespace Mantid
