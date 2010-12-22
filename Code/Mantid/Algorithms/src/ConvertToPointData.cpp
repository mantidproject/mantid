//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToPointData.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
  namespace Algorithms
  {

    DECLARE_ALGORITHM(ConvertToPointData);

    using API::MatrixWorkspace_sptr;
    using Mantid::MantidVec;

    //------------------------------------------------------------------------------
    // Private member functions
    //------------------------------------------------------------------------------
    
    /**
     * Returns true if the algorithm needs to be run. 
     * @returns True if the input workspace needs to be run through this algorithm
     */
    bool ConvertToPointData::isProcessingRequired(const MatrixWorkspace_sptr inputWS) const
    {
      if( !inputWS->isHistogramData() )
      {
     	g_log.information() << "Input workspace already contains point data. "
     			    << "OutputWorkspace set to InputWorkspace value.\n";
	return false;
      }
      return true;
    }
    
    /** 
     * Checks the input workspace's X data structure is logical.
     * @returns True if the X structure of the given input is what we expect, i.e. NX=NY+1
     */
    bool ConvertToPointData::isWorkspaceLogical(const MatrixWorkspace_sptr inputWS) const
    {
      const int numBins = inputWS->blocksize();
      const int numBoundaries = inputWS->readX(0).size();
      if( numBoundaries != (numBins + 1) )
      {
     	g_log.error() << "The number of bin boundaries must be one greater than the number of bins. "
     		      << "Found nbins=" << numBins << " and nBoundaries=" << numBoundaries << "\n";
	return false;
      }
      return true;
    }

    /**
     * Returns the size of the new X vector
     * @returns An integer giving the size of the new X vector
     */
    int ConvertToPointData::getNewXSize(const MatrixWorkspace_sptr inputWS) const
    {
      return inputWS->blocksize();
    }

    /**
     * Calculate the X point values
     * @param inputX A const reference to the input data
     * @param outputX A reference to the output data
     */
    void ConvertToPointData::calculateXPoints(const MantidVec & inputX, MantidVec &outputX) const
    {
      Kernel::VectorHelper::convertToBinCentre(inputX, outputX);
    }

    // /// Initialize the properties on the algorithm
    // void ConvertToPointData::init()
    // {
    //   using Kernel::Direction;
    //   declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    // 		      "Name of the input workspace.");
    //   declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    // 		      "Name of the output workspace, can be the same as the input." );
    // }

    // /// Execute the algorithm
    // void ConvertToPointData::exec()
    // {
    //   MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    //   // Firstly, do we need to do anything
    //   if( !inputWS->isHistogramData() )
    //   {
    // 	g_log.information() << "Input workspace already contains point data. "
    // 			    << "OutputWorkspace set to InputWorkspace value.\n";
    // 	setProperty("OutputWorkspace", inputWS);
    // 	return;
    //   }

    //   const int numBins = inputWS->blocksize();
    //   //Workspace guarantees that each X-vector is the same size
    //   const int numBoundaries = inputWS->readX(0).size();
    //   if( numBoundaries != (numBins + 1) )
    //   {
    // 	g_log.error() << "The number of bin boundaries must be one greater than the number of bins. "
    // 		      << "Found nbins=" << numBins << " and nBoundaries=" << numBoundaries << "\n";
    // 	throw std::runtime_error("Invalid X data structure.");
    //   }
    //   const int numSpectra = inputWS->getNumberHistograms();
    //   m_sharedX = API::WorkspaceHelpers::sharedXData(inputWS);

    //   // Create a new workspace where size(Y)=size(X)
    //   MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, numSpectra, numBins, numBins);

    //   Progress prog(this,0.0,1.0,numSpectra);
    //   PARALLEL_FOR2(inputWS,outputWS)
    //   for(int i = 0; i < numSpectra; ++i)
    //   {
    // 	PARALLEL_START_INTERUPT_REGION

    // 	// Copy over the Y and E data
    // 	outputWS->dataY(i) = inputWS->readY(i);
    // 	outputWS->dataE(i) = inputWS->readE(i);
    // 	setXData(outputWS, inputWS, i);
    // 	prog.report();

    // 	PARALLEL_END_INTERUPT_REGION
    //   }
    //   PARALLEL_CHECK_INTERUPT_REGION

    //   // Store the output
    //   setProperty("OutputWorkspace", outputWS);
    // }

    // /**
    //  * Set the X data on given spectra
    //  * @param outputWS The destination workspace
    //  * @param inputWS The input workspace
    //  * @param index The index
    //  */
    // void ConvertToPointData::setXData(API::MatrixWorkspace_sptr outputWS, 
    // 				      const API::MatrixWorkspace_sptr inputWS,
    // 				      const int index)
    // {
    //   if( m_sharedX )
    //   {
    // 	PARALLEL_CRITICAL(ConvertToPointData_para)
    // 	{
    // 	  if( (*m_cachedX).empty() )
    // 	  {
    // 	    PARALLEL_CRITICAL(ConvertToPointData_parb)
    // 	    {
    // 	      m_cachedX.access().resize(outputWS->blocksize());
    // 	      calculateXPoints(inputWS->readX(index), m_cachedX.access());
    // 	    }
    // 	  }
    // 	}
    // 	outputWS->setX(index, m_cachedX);
    //   }
    //   else
    //   {
    // 	const MantidVec & xBoundaries = inputWS->readX(index);
    // 	MantidVec & xPoints = outputWS->dataX(index);
    // 	calculateXPoints(xBoundaries, xPoints);
    //   }
      
    // }    
    
  }
}
