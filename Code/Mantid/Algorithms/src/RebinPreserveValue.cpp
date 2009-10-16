//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RebinPreserveValue.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
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
        new ArrayProperty<double>("Params", new RebinParamsValidator),
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
      const int histnumber = inputW->size()/inputW->blocksize();
      DataObjects::Histogram1D::RCtype XValues_new;
      // create new output X axis
      const int ntcnew = VectorHelper::createAxisFromRebinParams(rb_params,XValues_new.access());

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

  } // namespace Algorithm
} // namespace Mantid
