//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreatePSDBleedMask.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

#include <map>
#include <list>
#include <cfloat>
#include <iterator>

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
      m_maxRate(0.0), m_numIgnoredPixels(0), m_isRawCounts(false)
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
      declareProperty("NumberOfFailures", 0, new Kernel::NullValidator<int>(), 
		      "An output property containing the number of masked tubes", 
		      Direction::Output);      
    }

    /// Execute the algorithm
    void CreatePSDBleedMask::exec()
    {
      using Geometry::IDetector_sptr;

      MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
      // We require the number of good frames. Check that we have this
      if( !inputWorkspace->run().hasProperty("goodfrm") )
      {
	throw std::invalid_argument("InputWorkspace does not contain the number of \"good frames\".");
      }
      Kernel::PropertyWithValue<int> * frameProp = dynamic_cast<Kernel::PropertyWithValue<int>* >(inputWorkspace->run().getProperty("goodfrm"));
      assert(frameProp);
      int goodFrames = (*frameProp)();
      
      // Store the other properties
      double maxFramerate = getProperty("MaxTubeFramerate");
      // Multiply by the frames to save a division for each bin when we loop over them later
      m_maxRate = maxFramerate*goodFrames;
      m_numIgnoredPixels = getProperty("NIgnoredCentralPixels");

      // Check the input for being a distribution
      m_isRawCounts = !(inputWorkspace->isDistribution());

      // This algorithm assumes that the instrument geometry is tube based, i.e. the parent CompAssembly 
      // of the lowest detector in the tree is a "tube" and that all pixels in a tube are consectively ordered
      // with respect to spectra number
      const int numSpectra(inputWorkspace->getNumberHistograms());
      // Keep track of a map of tubes to lists of indices
      typedef std::map<Geometry::ComponentID, std::vector<int> > TubeIndex;
      TubeIndex tubeMap;
     
      // NOTE: This loop is intentionally left unparallelized as the majority of the
      // work requires a lock around it which actually slows down the loop.
      // Another benefit of keep it serial is losing the need for a call to 'sort' when
      // performing the bleed test as the list of indices will already be in the correct
      // order
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

	boost::shared_ptr<Geometry::DetectorGroup> group = 
	  boost::dynamic_pointer_cast<Geometry::DetectorGroup>(det);
	
	if( group )
	{
	  det = group->getDetectors().front();
	  if( !det ) continue;
	}
	boost::shared_ptr<const Geometry::IComponent> parent = det->getParent();
	if( !parent ) continue;

	Geometry::ComponentID parentID = parent->getComponentID();
	// Already have this component
	if( tubeMap.find(parentID) != tubeMap.end() )
	{
	  tubeMap[parentID].push_back(i);
	}
	// New tube
	else
	{
	  tubeMap.insert(std::pair<TubeIndex::key_type, TubeIndex::mapped_type>
			 (parentID, TubeIndex::mapped_type(1, i)));
	}
      }

      // Now process the tubes in parallel
      const int numTubes = tubeMap.size();
      g_log.information() << "Found " << numTubes << " tubes.\n";
      int numSpectraMasked(0), numTubesMasked(0);
      // Create a mask workspace for output
      MatrixWorkspace_sptr outputWorkspace = 
	API::WorkspaceFactory::Instance().create(inputWorkspace, numSpectra, 1, 1);
      outputWorkspace->setYUnit("");
      outputWorkspace->isDistribution(false);
      outputWorkspace->setYUnitLabel("MaskValue");

      PARALLEL_FOR2(inputWorkspace, outputWorkspace)
      for(int i = 0; i < numTubes; ++i )
      {
	PARALLEL_START_INTERUPT_REGION

	TubeIndex::iterator current = tubeMap.begin();
	std::advance(current, i);
	const TubeIndex::mapped_type tubeIndices = current->second;
	bool mask = performBleedTest(tubeIndices, inputWorkspace);
	if( mask )
	{
	  maskTube(tubeIndices, outputWorkspace);
	  PARALLEL_ATOMIC
	  numSpectraMasked += tubeIndices.size();
	  PARALLEL_ATOMIC
          numTubesMasked += 1;
	}
	else
	{
	  markAsPassed(tubeIndices, outputWorkspace);
	}
      
	PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      g_log.information() << numTubesMasked << " tube(s) failed the bleed tests.";
      if( numTubesMasked > 0 )
      {
	g_log.information() << " The " << numSpectraMasked << " spectra have been masked on the output workspace.\n";
      }
      else
      {
	g_log.information() << std::endl;
      }
      
      setProperty("NumberOfFailures", numSpectraMasked);
      setProperty("OutputWorkspace", outputWorkspace);
    }

    /**
     * Process a tube whose indices are given
     * @param tubeIndices A list of workspace indices that point to members of a single tube
     * @param inputWS The workspace containing the rates or counts for each bin
     * @returns True if the tube is to be masked, false otherwise
     */
    bool CreatePSDBleedMask::performBleedTest(const std::vector<int> & tubeIndices, 
					      API::MatrixWorkspace_const_sptr inputWS)
    {

      // Require ordered pixels so that we can define the centre.
      // This of course assumes that the pixel IDs increase monotonically with the workspace index
      // and that the above loop that searched for the tubes was NOT run in parallel
      const int numSpectra(tubeIndices.size());
      const int midIndex(numSpectra/2);
      const int topEnd(midIndex - m_numIgnoredPixels/2);
      const int bottomBegin(midIndex + m_numIgnoredPixels/2);

      const int numBins = inputWS->blocksize();
      std::vector<double> totalRate(numBins, 0.0);
      int top=0, bot=bottomBegin;
      for(; top < topEnd; ++top, ++bot )
      {
	const int topIndex = tubeIndices[top];
	const int botIndex = tubeIndices[bot];
	const MantidVec & topY = inputWS->readY(topIndex);
	const MantidVec & botY = inputWS->readY(botIndex);
	const MantidVec & topX = inputWS->readX(topIndex);
	const MantidVec & botX = inputWS->readX(botIndex);
	for( int j = 0; j < numBins; ++j)
	{
	  double topRate(topY[j]), botRate(botY[j]);
	  if(m_isRawCounts)
	  {
 	    topRate /= (topX[j+1] - topX[j]);
 	    botRate /= (botX[j+1] - botX[j]);
	  }
	  totalRate[j] += topRate + botRate;
	  // If by now any have hit the allowed maximum then mark this to be masked
	  if( totalRate[j] > m_maxRate )
	  {
	    return true;
	  }
	}
      }

      if (top != topEnd ) 
      {
	g_log.error() << "Error in tube processing, loop variable has an unexpected value.\n";
	throw std::runtime_error("top != topEnd in CreatePSDBleedMask::performBleedTest()");
      }
      if (bot != numSpectra) 
      {
	g_log.error() << "Error in tube processing, loop variable has an unexpected value.\n";
	throw std::runtime_error("bot != numSpectra  in CreatePSDBleedMask::performBleedTest()");
      }
      
      return false;
    }

    /** 
     * Mask a tube with the given workspace indices
     * @param tubeIndices A list of the workspaces indices for the tube
     * @param workspace The workspace to accumulate the masking
     */
    void CreatePSDBleedMask::maskTube(const std::vector<int> & tubeIndices, 
				      API::MatrixWorkspace_sptr workspace)
    {
      std::vector<int>::const_iterator cend = tubeIndices.end();
      for(std::vector<int>::const_iterator citr = tubeIndices.begin();
	  citr != cend; ++citr)
      {
	workspace->maskWorkspaceIndex(*citr);
      }
    }

    /**
     * Mark a tube's data values as passing the tests
     * @param tubeIndices A list of the workspaces indices for the tube
     * @param workspace The workspace's data values to change
     */
    void CreatePSDBleedMask::markAsPassed(const std::vector<int> & tubeIndices, 
					  API::MatrixWorkspace_sptr workspace)
    {
      // Mark as unmasked
      std::vector<int>::const_iterator cend = tubeIndices.end();
      for( std::vector<int>::const_iterator citr = tubeIndices.begin();
	   citr != cend; ++citr )
      {
	workspace->dataY(*citr)[0] = 1.0;
	workspace->dataE(*citr)[0] = 1.0;
      }
    }

  }

}

