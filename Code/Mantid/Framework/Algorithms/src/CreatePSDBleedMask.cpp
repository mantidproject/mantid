//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreatePSDBleedMask.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"

#include <map>
#include <list>
#include <cfloat>

namespace Mantid
{
  namespace Algorithms
  {
    
    // Register the class
    DECLARE_ALGORITHM(CreatePSDBleedMask)

    using API::MatrixWorkspace_sptr;
    using API::MatrixWorkspace_const_sptr;

    //----------------------------------------------------------------------
    // Public methods
    //----------------------------------------------------------------------
    
    /// Default constructor
    CreatePSDBleedMask::CreatePSDBleedMask() : 
      m_maxFramerate(0.0), m_numIgnoredPixels(0), m_goodFrames(0)
    {
    }

    //----------------------------------------------------------------------
    // Private methods
    //----------------------------------------------------------------------

    /// Initialize the algorithm properties
    void CreatePSDBleedMask::init()
    {
      using API::WorkspaceProperty;
      using Kernel::Direction;
      using Kernel::BoundedValidator;

      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
		      "The name of the input workspace.");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
		      "The name of the output MaskWorkspace which will contain the result masks.");
      BoundedValidator<double> * mustBePosDbl = new BoundedValidator<double>();
      mustBePosDbl->setLower(0.0);
      declareProperty("MaxTubeFramerate", -1.0, mustBePosDbl,
		      "The maximum rate allowed for a tube in counts/us/frame.");
      BoundedValidator<int> * mustBePosInt = new BoundedValidator<int>();
      mustBePosInt->setLower(0);
      declareProperty("NIgnoredCentralPixels", 80, mustBePosInt,
		      "The number of pixels about the centre to ignore.");      
    }

    /// Execute the algorithm
    void CreatePSDBleedMask::exec()
    {
      using Geometry::IDetector_sptr;

      MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
      // We require the number of good frames. Check that we have this
      if( !inputWorkspace->run().hasProperty("goodfrm") )
      {
	throw std::invalid_argument("InputWorkspace does not contain the number of \"good frames\".");
      }

      // Store the other properties
      m_maxFramerate = getProperty("MaxTubeFramerate");
      m_numIgnoredPixels = getProperty("NIgnoredCentralPixels");
      
      // As input to the algorithm we need an integrated count rate. Check what we need to do to get this
      // If the input workspace has 1 bin then assume it has been integrated
      MatrixWorkspace_sptr outputWorkspace;
      if( inputWorkspace->blocksize() > 1 )
      {
	outputWorkspace = this->integrateSpectra(inputWorkspace, EMPTY_INT(), EMPTY_INT(),
						 EMPTY_DBL(), EMPTY_DBL(), true);
      }
      else
      {
	outputWorkspace = this->cloneWorkspace(inputWorkspace);
      }
      // Convert to count rate
      outputWorkspace = convertToRate(outputWorkspace);

      Kernel::PropertyWithValue<int> * frameProp = 
	dynamic_cast<Kernel::PropertyWithValue<int>* >(inputWorkspace->run().getProperty("goodfrm"));
      assert(frameProp);
      m_goodFrames = (*frameProp)();
      // This algorithm assumes that the instrument geometry is tube based, i.e. the parent CompAssembly 
      // of the lowest detector in the tree is a "tube" and that all pixels in a tube are consectively ordered
      // with respect to spectra number
      const int numSpectra(inputWorkspace->getNumberHistograms());

      // Keep track of a map of tubes to lists of indices
      std::map<Geometry::ComponentID, std::list<int> > tubeMap;

      for( int i = 0; i < numSpectra; ++i )
      {

	IDetector_sptr det;
	try
	{
	  det = inputWorkspace->getDetector(i);
	}
	catch( Kernel::Exception::NotFoundError & )
	{
	  continue;
	}
	if( det->isMonitor() ) continue;

	Geometry::ComponentID parentID = det->getParent()->getComponentID();
	// Already have this component
	if( tubeMap.find(parentID) != tubeMap.end() )
	{
	  PARALLEL_CRITICAL(CreatePSDBleedMask_A)
	  {
	    tubeMap[parentID].push_back(i);
	  }
	}
	// New one
	else
	{
	  PARALLEL_CRITICAL(CreatePSDBleedMask_B)
	  {
	    tubeMap.insert(std::pair<Geometry::ComponentID, std::list<int> >
			   (parentID, std::list<int>(1, i)));
	  }
	}

      }

      // Now process the tubes
      const int numTubes = tubeMap.size();
      std::map<Geometry::ComponentID, std::list<int> >::iterator citr = tubeMap.begin();
      for(int i = 0; i < numTubes; ++i )
      {
	performBleedTest(citr->second, outputWorkspace);
	// Move to next tube
	++citr;
      }
            
      setProperty("OutputWorkspace", outputWorkspace);
    }

    /**
     * Process a tube whose indices are given
     * @param tubeIndices A list of workspace indices that point to members of a single tube
     * @param rateWS The workspace containing the count rates for each spectra 
     */
    void CreatePSDBleedMask::performBleedTest(std::list<int> & tubeIndices, 
					      API::MatrixWorkspace_sptr rateWS)
    {
      // Require ordered pixels so that we can define the centre.
      // This of course assumes that the pixel IDs increase monotonically with the workspace index
      tubeIndices.sort();
      const int numPixels = tubeIndices.size();

      const int midIndex(numPixels/2);
      const int topEndPixel(midIndex - m_numIgnoredPixels/2);
      const int bottomStartPixel(midIndex + m_numIgnoredPixels/2);
      
      // Find maximum value
      double maxCountRate(-DBL_MAX);
      std::list<int>::const_iterator citr = tubeIndices.begin();
      // Top half
      for(int i = 0; i < topEndPixel; ++i )
      {
	const double & countRate = rateWS->readY(*citr)[0];
	if( countRate > maxCountRate ) maxCountRate = countRate;
	++citr;
      }
      // Bottom half
      for(int i = bottomStartPixel; i < numPixels; ++i )
      {
	const double & countRate = rateWS->readY(*citr)[0];
	if( countRate > maxCountRate ) maxCountRate = countRate;
	++citr;
      }

      if( (maxCountRate/m_goodFrames) > m_maxFramerate )
      {
	maskTube(tubeIndices, rateWS);
      }
    }

    /// Mask a tube with the given workspace indices
    void CreatePSDBleedMask::maskTube(const std::list<int> & tubeIndices, 
				      API::MatrixWorkspace_sptr workspace)
    {
      std::list<int>::const_iterator cend = tubeIndices.end();
      for(std::list<int>::const_iterator citr = tubeIndices.begin();
	  citr != cend; ++citr)
      {
	workspace->maskWorkspaceIndex(*citr);
      }
    }

    /**
     * Run the CloneWorkspace algorithm
     * @param workspace The input workspace to clone
     */
    MatrixWorkspace_sptr CreatePSDBleedMask::cloneWorkspace(MatrixWorkspace_sptr workspace)
    {
      double t0 = m_fracDone, t1 = advanceProgress(RTGetTotalCounts);
      API::IAlgorithm_sptr cloner = createSubAlgorithm("CloneWorkspace", t0, t1);
      cloner->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
      cloner->execute();
      return cloner->getProperty("OutputWorkspace");
    }

  }

}

