//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RebinPreserveValue.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(RebinPreserveValue)

    using namespace Kernel;
    using namespace API;
    using DataObjects::Workspace2D;
    using DataObjects::Workspace2D_sptr;

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void RebinPreserveValue::init()
    {  
      CompositeValidator<> *wsValidator = new CompositeValidator<>;
      wsValidator->add(new HistogramValidator<>);
      //wsValidator->add(new RawCountValidator<>);
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
        "The name of the Workspace2D to take as input" );
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace to be created as the output of the algorithm" );
      declareProperty(
        new ArrayProperty<double>("Params", new MandatoryValidator<std::vector<double> >),
        "The new bin widths in the form x1, deltax1, x2, deltax2, x3, ..." );
    }

    /** Executes the rebin algorithm
    *
    */
    void RebinPreserveValue::exec()
    {
      // retrieve the properties
      std::vector<double> rb_params=getProperty("Params");

      // Get the input workspace
      MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");
      // The input workspace has to have just one bin per spectrum
      if ( inputW->blocksize() > 1 )
      {
        g_log.error("The input workspace is restricted to having only one bin");
        throw std::invalid_argument("The input workspace is restricted to having only one bin");
      }

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
        const std::vector<double>& XValues = inputW->readX(hist);
        const std::vector<double>& YValues = inputW->readY(hist);
        const std::vector<double>& YErrors = inputW->readE(hist);

        //get references to output workspace data (no copying)
        std::vector<double>& YValues_new=outputW->dataY(hist);
        std::vector<double>& YErrors_new=outputW->dataE(hist);

        // output data arrays are implicitly filled by function
        VectorHelper::rebinNonDispersive(XValues,YValues,YErrors,*XValues_new,YValues_new,YErrors_new,false);
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
        if (hist % progress_step == 0)
        {
            progress(double(hist)/histnumber);
            interruption_point();
        }
      }

      // Copy units
      if (outputW->getAxis(0)->unit().get())
          outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
      try
      {
          if (inputW->getAxis(1)->unit().get())
              outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
      }
      catch(Exception::IndexError) {
          // OK, so this isn't a Workspace2D
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
    int RebinPreserveValue::newAxis(const std::vector<double>& params, std::vector<double>& xnew)
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

  } // namespace Algorithm
} // namespace Mantid
