//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToPointData.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
  namespace Algorithms
  {

    DECLARE_ALGORITHM(ConvertToPointData);

    using API::HistogramValidator;
    using API::WorkspaceProperty;
    using API::MatrixWorkspace_sptr;
    using API::MatrixWorkspace_const_sptr;
    using API::WorkspaceFactory;
    using Mantid::MantidVecPtr;

    //------------------------------------------------------------------------------
    // Public member functions
    //------------------------------------------------------------------------------
    /** 
     * Default constructor
     */
    ConvertToPointData::ConvertToPointData() : m_sharedX(false), m_cachedX()
    {
    }

    //------------------------------------------------------------------------------
    // Private member functions
    //------------------------------------------------------------------------------
    
    /// Initialize the properties on the algorithm
    void ConvertToPointData::init()
    {
      using Kernel::Direction;
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>),
		      "Name of the input workspace containing histogram data");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
		      "Name of the output workspace, can be the same as the input" );
    }

    /// Execute the algorithm
    void ConvertToPointData::exec()
    {
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      const int numBins = inputWS->blocksize();
      const int numSpectra = inputWS->getNumberHistograms();
      m_sharedX = API::WorkspaceHelpers::sharedXData(inputWS);

      // Create a new workspace with one less bin value
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, numSpectra, numBins, numBins);
      
      PARALLEL_FOR2(inputWS,outputWS)
      for(int i = 0; i < numSpectra; ++i)
      {
	PARALLEL_START_INTERUPT_REGION

	// Copy over the Y and E data
	outputWS->dataY(i) = inputWS->readY(i);
	outputWS->dataE(i) = inputWS->readE(i);
	setXData(outputWS, inputWS, i);

	PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Store the output
      setProperty("OutputWorkspace", outputWS);
    }

    /**
     * Set the X data on given spectra
     * @param outputWS The destination workspace
     * @param inputWS The input workspace
     * @param index The index
     */
    void ConvertToPointData::setXData(API::MatrixWorkspace_sptr outputWS, 
				      const API::MatrixWorkspace_sptr inputWS,
				      const int index)
    {
      if( m_sharedX )
      {
	PARALLEL_CRITICAL(ConvertToPointData_para)
	{
	  if( (*m_cachedX).empty() )
	  {
	    PARALLEL_CRITICAL(ConvertToPointData_parb)
	    {
	      m_cachedX.access().resize(outputWS->blocksize());
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
    
    /**
     * Calculate the X point values
     * @param inputX A const reference to the input data
     * @param outputX A reference to the output data
     */
    void ConvertToPointData::calculateXPoints(const MantidVec & inputX, MantidVec &outputX) const
    {
      // Adjust the X bins
      MantidVec::iterator pointItr = outputX.begin();
      MantidVec::const_iterator pointEnd = outputX.end();
      MantidVec::const_iterator histItr = inputX.begin();
      for( ; pointItr != pointEnd; ++pointItr, ++histItr )
      {
	(*pointItr) = 0.5*(*histItr + *(histItr+1));
      }
      
    }
  }
}
