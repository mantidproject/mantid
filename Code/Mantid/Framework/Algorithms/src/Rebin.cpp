/*WIKI* 

The algorithm rebins data with new bin boundaries. The 'params' property defines new boundaries in intervals <math>x_i-x_{i+1}\,</math>. Positive <math>\Delta x_i\,</math> make constant width bins, whilst negative ones create logarithmic binning using the formula <math>x(j+1)=x(j)(1+|\Delta x_i|)\,</math>

This algorithms is useful both in data reduction, but also in remapping [[Ragged Workspace|ragged workspaces]] to a regular set of bin boundaries.

The bin immediately before the specified boundaries <math>x_2</math>, <math>x_3</math>, ... <math>x_i</math> is likely to have a different width from its neighbours because there can be no gaps between bins. Rebin ensures that any of these space filling bins cannot be less than 25% or more than 125% of the width that was specified.

=== Example Rebin param strings ===
;-0.0001
:From min(TOF) to max(TOF) among all events in Logarithmic bins of 0.0001
;0,100,20000
:From 0 rebin in constant size bins of 100 up to 20,000
;2,-0.035,10
:From 10 rebin in Logarithmic bins of 0.035 up to 10
;0,100,10000,200,20000
:From 0 rebin in steps of 100 to 10,000 then steps of 200 to 20,000

=== For EventWorkspaces ===

If the input is an [[EventWorkspace]] and the "Preserve Events" property is True, the rebinning is performed in place, and only the X axes of the workspace are set. The actual Y histogram data will only be requested as needed, for example, when plotting or displaying the data. 

If "Preserve Events" is false., then the output workspace will be created as a [[Workspace2D]], with fixed histogram bins, and all Y data will be computed immediately. All event-specific data is lost at that point.

=== For Data-Point Workspaces ===

If the input workspace contains data points, rather than histograms, then Rebin will automatically use the [[ConvertToHistogram]] and [[ConvertToPointData]] algorithms before and after the rebinning has taken place.


*WIKI*/
/*WIKI_USAGE*
'''Python'''
 outputW = Rebin("inputW","x1,dx1,x2")

*WIKI_USAGE*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Rebin)
    
    /// Sets documentation strings for this algorithm
    void Rebin::initDocs()
    {
      this->setWikiSummary("Rebins data with new X bin boundaries. For EventWorkspaces, you can very quickly rebin in-place by keeping the same output name and PreserveEvents=true.");
      this->setOptionalMessage("Rebins data with new X bin boundaries. For EventWorkspaces, you can very quickly rebin in-place by keeping the same output name and PreserveEvents=true.");
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
    void Rebin::init()
    {
      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
        "Workspace containing the input data");
      declareProperty(
        new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "The name to give the output workspace");

      declareProperty(
        new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>()),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally "
        "this can be followed by a comma and more widths and last boundary pairs. "
        "Optionally this can also be a single number, which is the bin width. "
        "In this case, the boundary of binning will be determined by minimum and maximum TOF "
        "values among all events, or previous binning boundary, in case of event Workspace, or "
        "non-event Workspace, respectively. Negative width values indicate logarithmic binning. ");

      declareProperty("PreserveEvents", true,"Keep the output workspace as an EventWorkspace, "
                      "if the input has events (default). If the input and output EventWorkspace "
                      "names are the same, only the X bins are set, which is very quick. If false, "
                      "then the workspace gets converted to a Workspace2D histogram.");

    }


    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void Rebin::exec()
    {
      // Get the input workspace
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

      // Are we preserving event workspace-iness?
      bool PreserveEvents = getProperty("PreserveEvents");

      // Rebinning in-place
      bool inPlace = (inputWS == outputWS);

      // retrieve the properties
      const std::vector<double> in_params=getProperty("Params");
      std::vector<double> rb_params;

      // The validator only passes parameters with size 1, or 3xn.  No need to check again here
      if (in_params.size() >= 3){
        // Input are min, delta, max
        rb_params = in_params;

      } else if (in_params.size() == 1){
        double xmin = 0.;
        double xmax = 0.;
        inputWS->getXMinMax(xmin, xmax);

        g_log.information() << "Using the current min and max as default " << xmin << ", " << xmax << std::endl;

        rb_params.push_back(xmin);
        rb_params.push_back(in_params[0]);
        rb_params.push_back(xmax);

      }

      const bool dist = inputWS->isDistribution();

      const bool isHist = inputWS->isHistogramData();

      // workspace independent determination of length
      const int histnumber = static_cast<int>(inputWS->getNumberHistograms());
      MantidVecPtr XValues_new;
      // create new output X axis
      const int ntcnew = VectorHelper::createAxisFromRebinParams(rb_params, XValues_new.access());

      //---------------------------------------------------------------------------------
      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_const_sptr eventInputWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

      if (eventInputWS != NULL)
      {
        //------- EventWorkspace as input -------------------------------------
        EventWorkspace_sptr eventOutputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);

        if (inPlace && PreserveEvents)
        {
          // -------------Rebin in-place, preserving events ----------------------------------------------
          // This only sets the X axis. Actual rebinning will be done upon data access.
          eventOutputWS->setAllX(XValues_new);
          this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(eventOutputWS));
        }
        else if (!inPlace && PreserveEvents)
        {
          // -------- NOT in-place, but you want to keep events for some reason. ----------------------
          // Must copy the event workspace to a new EventWorkspace (and bin that).

          //Make a brand new EventWorkspace
          eventOutputWS = boost::dynamic_pointer_cast<EventWorkspace>(
              API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
          //Copy geometry over.
          API::WorkspaceFactory::Instance().initializeFromParent(inputWS, eventOutputWS, false);
          //You need to copy over the data as well.
          eventOutputWS->copyDataFrom( (*eventInputWS) );

          // This only sets the X axis. Actual rebinning will be done upon data access.
          eventOutputWS->setAllX(XValues_new);

          //Cast to the matrixOutputWS and save it
          this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(eventOutputWS));
        }
        else
        {
          //--------- Different output, OR you're inplace but not preserving Events --- create a Workspace2D -------
          g_log.information() << "Creating a Workspace2D from the EventWorkspace " << eventInputWS->getName() << ".\n";

          //Create a Workspace2D
          // This creates a new Workspace2D through a torturous route using the WorkspaceFactory.
          // The Workspace2D is created with an EMPTY CONSTRUCTOR
          outputWS = WorkspaceFactory::Instance().create("Workspace2D",histnumber,ntcnew,ntcnew-1);
          WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, true);

          //Initialize progress reporting.
          Progress prog(this,0.0,1.0, histnumber);

          //Go through all the histograms and set the data
          PARALLEL_FOR3(inputWS, eventInputWS, outputWS)
          for (int i=0; i < histnumber; ++i)
          {
            PARALLEL_START_INTERUPT_REGION

            //Set the X axis for each output histogram
            outputWS->setX(i, XValues_new);

            //Get a const event list reference. eventInputWS->dataY() doesn't work.
            const EventList& el = eventInputWS->getEventList(i);
            MantidVec y_data, e_data;
            // The EventList takes care of histogramming.
            el.generateHistogram(*XValues_new, y_data, e_data);

            //Copy the data over.
            outputWS->dataY(i).assign(y_data.begin(), y_data.end());
            outputWS->dataE(i).assign(e_data.begin(), e_data.end());

            //Report progress
            prog.report(name());
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION

          //Copy all the axes
          for (int i=1; i<inputWS->axes(); i++)
          {
            outputWS->replaceAxis( i, inputWS->getAxis(i)->clone(outputWS.get()) );
            outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
          }

          //Copy the units over too.
          for (int i=0; i < outputWS->axes(); ++i)
            outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
          outputWS->setYUnit(eventInputWS->YUnit());
          outputWS->setYUnitLabel(eventInputWS->YUnitLabel());

          // Assign it to the output workspace property
          setProperty("OutputWorkspace", outputWS);
        }

      } // END ---- EventWorkspace

      else

      { //------- Workspace2D or other MatrixWorkspace ---------------------------

        if ( ! isHist )
        {
          g_log.information() << "Rebin: Converting Data to Histogram." << std::endl;
          Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ConvertToHistogram");
          ChildAlg->initialize();
          ChildAlg->setProperty("InputWorkspace", inputWS);
          ChildAlg->execute();
          inputWS = ChildAlg->getProperty("OutputWorkspace");
        }

        // This will be the output workspace (exact type may vary)
        API::MatrixWorkspace_sptr outputWS;

        // make output Workspace the same type is the input, but with new length of signal array
        outputWS = API::WorkspaceFactory::Instance().create(inputWS,histnumber,ntcnew,ntcnew-1);


        // Copy over the 'vertical' axis
        if (inputWS->axes() > 1) outputWS->replaceAxis( 1, inputWS->getAxis(1)->clone(outputWS.get()) );

        Progress prog(this,0.0,1.0,histnumber);
        PARALLEL_FOR2(inputWS,outputWS)
        for (int hist=0; hist <  histnumber;++hist)
        {
          PARALLEL_START_INTERUPT_REGION
          // get const references to input Workspace arrays (no copying)
          const MantidVec& XValues = inputWS->readX(hist);
          const MantidVec& YValues = inputWS->readY(hist);
          const MantidVec& YErrors = inputWS->readE(hist);

          //get references to output workspace data (no copying)
          MantidVec& YValues_new=outputWS->dataY(hist);
          MantidVec& YErrors_new=outputWS->dataE(hist);

          // output data arrays are implicitly filled by function
          try {
            VectorHelper::rebin(XValues,YValues,YErrors,*XValues_new,YValues_new,YErrors_new, dist);
          } catch (std::exception& ex)
          {
            g_log.error() << "Error in rebin function: " << ex.what() << std::endl;
            throw;
          }

          // Populate the output workspace X values
          outputWS->setX(hist,XValues_new);

          prog.report(name());
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
        outputWS->isDistribution(dist);

        // Now propagate any masking correctly to the output workspace
        // More efficient to have this in a separate loop because
        // MatrixWorkspace::maskBins blocks multi-threading
        for (int i=0; i <  histnumber; ++i)
        {
          if ( inputWS->hasMaskedBins(i) )  // Does the current spectrum have any masked bins?
          {
            this->propagateMasks(inputWS,outputWS,i);
          }
        }
        //Copy the units over too.
        for (int i=0; i < outputWS->axes(); ++i)
        {
          outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
        }

        if ( ! isHist )
        {
          g_log.information() << "Rebin: Converting Data back to Data Points." << std::endl;
          Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ConvertToPointData");
          ChildAlg->initialize();
          ChildAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
          ChildAlg->execute();
          outputWS = ChildAlg->getProperty("OutputWorkspace");
        }

        // Assign it to the output workspace property
        setProperty("OutputWorkspace",outputWS);


      } // END ---- Workspace2D

      return;
    }

//
//    /** Continue execution for EventWorkspace scenario */
//    void Rebin::execEvent()
//    {
//      // retrieve the properties
//      std::vector<double> rb_params=getProperty("Params");
//
//    }

    /** Takes the masks in the input workspace and apportions the weights into the new bins that overlap
     *  with a masked bin. These bins are then masked with the calculated weight.
     * 
     *  @param inputWS ::  The input workspace
     *  @param outputWS :: The output workspace
     *  @param hist ::    The index of the current histogram
     */
    void Rebin::propagateMasks(API::MatrixWorkspace_const_sptr inputWS, API::MatrixWorkspace_sptr outputWS, int hist)
    {
      // Not too happy with the efficiency of this way of doing it, but it's a lot simpler to use the
      // existing rebin algorithm to distribute the weights than to re-implement it for this
      
      MantidVec masked_bins,weights;
      // Get a reference to the list of masked bins for this spectrum
      const MatrixWorkspace::MaskList& mask = inputWS->maskedBins(hist);
      // Now iterate over the list, building up a vector of the masked bins
      MatrixWorkspace::MaskList::const_iterator it = mask.begin();
      const MantidVec& XValues = inputWS->readX(hist);
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
      const MantidVec& XValues_new = outputWS->readX(hist);
      MantidVec newWeights(XValues_new.size()-1),zeroes2(XValues_new.size()-1);
      // Use rebin function to redistribute the weights. Note that distribution flag is set
      VectorHelper::rebin(masked_bins,weights,zeroes,XValues_new,newWeights,zeroes2,true);
      
      // Now process the output vector and fill the new masking list
      for (size_t index = 0; index < newWeights.size(); ++index)
      {
        if ( newWeights[index] > 0.0 ) outputWS->flagMasked(hist, index, newWeights[index]);
      }
    }
    
  } // namespace Algorithm
} // namespace Mantid
