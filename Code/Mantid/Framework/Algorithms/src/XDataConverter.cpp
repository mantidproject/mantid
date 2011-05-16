//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/XDataConverter.h"

namespace Mantid
{
  namespace Algorithms
  {

    using API::WorkspaceProperty;
    using API::MatrixWorkspace_sptr;
    using API::MatrixWorkspace_const_sptr;
    using API::WorkspaceFactory;
    using API::Progress;
    using Mantid::MantidVecPtr;

    //------------------------------------------------------------------------------
    // Public member functions
    //------------------------------------------------------------------------------
    /** 
     * Default constructor
     */
    XDataConverter::XDataConverter() : m_sharedX(false), m_cachedX()
    {
    }

    //------------------------------------------------------------------------------
    // Private member functions
    //------------------------------------------------------------------------------
    
    /// Initialize the properties on the algorithm
    void XDataConverter::init()
    {
      using Kernel::Direction;
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
		      "Name of the input workspace.");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
		      "Name of the output workspace, can be the same as the input." );
    }

    /// Execute the algorithm
    void XDataConverter::exec()
    {
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      if( !isProcessingRequired(inputWS) )
      {
	setProperty("OutputWorkspace", inputWS);
	return;
      }
      if( !isWorkspaceLogical(inputWS) )
      {
	throw std::runtime_error("Invalid InputWorkspace data structure. Check log for details.");
      }

      const int numSpectra = static_cast<int>(inputWS->getNumberHistograms());
      const size_t numYValues = inputWS->blocksize();
      const int numXValues = getNewXSize(inputWS);
      m_sharedX = API::WorkspaceHelpers::sharedXData(inputWS);
      // Create the new workspace 
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, numSpectra, numXValues, numYValues);

      Progress prog(this,0.0,1.0,numSpectra);
      PARALLEL_FOR2(inputWS,outputWS)
      for(int i = 0; i < numSpectra; ++i)
      {
	PARALLEL_START_INTERUPT_REGION

	// Copy over the Y and E data
	outputWS->dataY(i) = inputWS->readY(i);
	outputWS->dataE(i) = inputWS->readE(i);
	setXData(outputWS, inputWS, i);
	prog.report();

	PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Store the output
      setProperty("OutputWorkspace", outputWS);
    }

    /**
     * Set the X data on given spectra
     * @param outputWS :: The destination workspace
     * @param inputWS :: The input workspace
     * @param index :: The index
     */
    void XDataConverter::setXData(API::MatrixWorkspace_sptr outputWS, 
				  const API::MatrixWorkspace_sptr inputWS,
				  const int index)
    {
      if( m_sharedX )
      {
	PARALLEL_CRITICAL(XDataConverter_para)
	{
	  if( (*m_cachedX).empty() )
	  {
	    PARALLEL_CRITICAL(XDataConverter_parb)
	    {
	      m_cachedX.access().resize(getNewXSize(inputWS));
	      calculateXPoints(inputWS->readX(index), m_cachedX.access());
	    }
	  }
	}
	outputWS->setX(index, m_cachedX);
      }
      else
      {
	const MantidVec & xBoundaries = inputWS->readX(index);
	MantidVec & xPoints = outputWS->dataX(index);
	calculateXPoints(xBoundaries, xPoints);
      }
      
    }    
  }
}
