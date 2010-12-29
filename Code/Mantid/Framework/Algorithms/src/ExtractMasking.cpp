//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ExtractMasking.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ExtractMasking)

    using Kernel::Direction;
    using Geometry::IDetector_sptr;
    using namespace API;

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------

    /**
     * Declare the algorithm properties
     */
    void ExtractMasking::init()
    {
      declareProperty(
	new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",
					       Direction::Input)
	);
      declareProperty(
	new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",
					       Direction::Output));
    }

    /**
     * Execute the algorithm
     */
    void ExtractMasking::exec()
    {
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
      // First check that we have got a non-empty spectra-detector map. If not, getDetector will definitely fail
      if( inputWS->spectraMap().nElements() == 0 )
      {
	throw std::invalid_argument("Invalid input workspace, the spectra map is not populated.");
      }

      const int nHist = inputWS->getNumberHistograms();
      const int xLength(1), yLength(1);
      // Create a new workspace for the results, copy from the input to ensure that we copy over the instrument and current masking
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, nHist, xLength, yLength);
      // The size has changed so the axes are not copied over by default. We need the spectra axis though.
      // The X axis is irrelevant here.
      outputWS->replaceAxis(1, inputWS->getAxis(1)->clone()); 
     
      Progress prog(this,0.0,1.0,nHist);
      MantidVecPtr xValues;
      xValues.access() = MantidVec(1, 0.0);

      PARALLEL_FOR2(inputWS, outputWS)
      for( int i = 0; i < nHist; ++i )
      {
	PARALLEL_START_INTERUPT_REGION

      	outputWS->setX(i, xValues);
      	IDetector_sptr inputDet;
      	bool inputIsMasked(false);
      	try
      	{
      	  inputDet = inputWS->getDetector(i);
      	  if( inputDet->isMasked() )
      	  {
      	    inputIsMasked = true;
      	  }
      	}
      	catch(Kernel::Exception::NotFoundError &)
      	{
      	  inputIsMasked = false;
      	}

      	if( inputIsMasked )
      	{
      	  outputWS->dataY(i)[0] = 0.0;
      	  outputWS->dataE(i)[0] = 0.0;
      	}
      	else
      	{
      	  outputWS->dataY(i)[0] = 1.0;
      	  outputWS->dataE(i)[0] = 1.0;
      	}
      	prog.report();

	PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      setProperty("OutputWorkspace", outputWS);
    }
    
  }
}

