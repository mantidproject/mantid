//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleRebin.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"

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

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void SimpleRebin::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output));

      declareProperty(new ArrayProperty<double>("params", new MandatoryValidator<std::vector<double> >));
    }

    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void SimpleRebin::exec()
    {
      // retrieve the properties
      std::vector<double> rb_params=getProperty("params");

      // Get the input workspace
      MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

      bool dist = inputW->isDistribution();

      // workspace independent determination of length
      int histnumber = inputW->size()/inputW->blocksize();
      DataObjects::Histogram1D::RCtype XValues_new;
      // create new output X axis
      int ntcnew = newAxis(rb_params,XValues_new.access());

      // make output Workspace the same type is the input, but with new length of signal array
      API::MatrixWorkspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,ntcnew,ntcnew-1);
      // Try to cast it to a Workspace2D for use later
      Workspace2D_sptr outputW_2D = boost::dynamic_pointer_cast<Workspace2D>(outputW);

      int progress_step = histnumber / 100;
      if (progress_step == 0) progress_step = 1;
      for (int hist=0; hist <  histnumber;++hist)
      {
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
        if (outputW_2D)
        {
          outputW_2D->setX(hist,XValues_new);
        }
        else
        {
          outputW->dataX(hist)=XValues_new.access();
        }
        //copy oer the spectrum No and ErrorHelper
        try {
          outputW->getAxis(1)->spectraNo(hist)=inputW->getAxis(1)->spectraNo(hist);
        } catch (Exception::IndexError) {
          // OK, so this isn't a Workspace2D
        }
        
        // Now propagate any masking correctly to the output workspace
        if ( inputW->hasMaskedBins(hist) )  // Does the current spectrum have any masked bins?
        {
          this->propagateMasks(inputW,outputW,hist);
        }
        
        if (hist % progress_step == 0)
        {
          progress(double(hist)/histnumber);
          interruption_point();
        }
      }
      outputW->isDistribution(dist);

      for (int i=0; i < outputW->axes(); ++i)
      {
        outputW->getAxis(i)->unit() = inputW->getAxis(i)->unit();        
      }
      
      // Assign it to the output workspace property
      setProperty("OutputWorkspace",outputW);

      return;
    }

    /** Creates a new  output X array  according to specific boundary defnitions
     *
     *  @param params - rebin parameters input [x_1, delta_1,x_2, ... ,x_n-1,delta_n-1,x_n)
     *  @param xnew - new output workspace x array
     *  @return The number of bin boundaries in the new X array
     **/
    int SimpleRebin::newAxis(const std::vector<double>& params, std::vector<double>& xnew)
    {
      double xcurr, xs;
      int ibound(2), istep(1), inew(1);
      int ibounds=params.size(); //highest index in params array containing a bin boundary
      int isteps=ibounds-1; // highest index in params array containing a step
      xnew.clear();
      
      xcurr = params[0];
      xnew.push_back(xcurr);

      while( (ibound <= ibounds) && (istep <= isteps) )
      {
        // if step is negative then it is logarithmic step
        if ( params[istep] >= 0.0)
          xs = params[istep];
        else
          xs = xcurr * fabs(params[istep]);
        /* continue stepping unless we get to almost where we want to */
        if ( (xcurr + xs) < (params[ibound] - (xs * 1.E-6)) )
        {
          xcurr += xs;
        }
        else
        {
          xcurr = params[ibound];
          ibound+=2;
          istep+=2;
        }
        xnew.push_back(xcurr);
        inew++;
      }
      
      // If the last bin is smaller than 25% of the penultimate one, then combine the last two
      if ( inew > 2 && (xnew[inew-1]-xnew[inew-2]) < 0.25*(xnew[inew-2]-xnew[inew-3]) )
      {
        xnew.erase(xnew.end()-2);
        --inew;
      }
      
      return inew;
    }

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
