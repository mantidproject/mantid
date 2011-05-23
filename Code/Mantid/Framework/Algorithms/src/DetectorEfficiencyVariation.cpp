//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorEfficiencyVariation.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <boost/math/special_functions/fpclassify.hpp>

// togo
#include "MantidAlgorithms/InputWSDetectorInfo.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(DetectorEfficiencyVariation)
    
    /// Sets documentation strings for this algorithm
    void DetectorEfficiencyVariation::initDocs()
    {
      this->setWikiSummary("Compares two white beam vanadium workspaces from the same instrument to find detectors whose efficiencies have changed beyond a threshold. ");
      this->setOptionalMessage("Compares two white beam vanadium workspaces from the same instrument to find detectors whose efficiencies have changed beyond a threshold.");
    }
    

    using namespace Kernel;
    using namespace API;
    using Geometry::IDetector_sptr;

    /// Default constructor initialises all data members and runs the base class constructor
    DetectorEfficiencyVariation::DetectorEfficiencyVariation() : 
      DetectorDiagnostic(), m_usableMaskMap(true)
    {}

    /// Initialize the algorithm
    void DetectorEfficiencyVariation::init()
    {
      HistogramValidator<MatrixWorkspace> *val =
	new HistogramValidator<MatrixWorkspace>;
      declareProperty(
	new WorkspaceProperty<MatrixWorkspace>("WhiteBeamBase", "",
					       Direction::Input,val),
	"Name of a white beam vanadium workspace" );
      // The histograms, the detectors in each histogram and their first and last bin boundary must match
      declareProperty(
	new WorkspaceProperty<MatrixWorkspace>("WhiteBeamCompare","",Direction::Input,
					       val->clone()),
	"Name of a matching second white beam vanadium run from the same\n"
	"instrument" );
      declareProperty(
	new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
	"A MaskWorkpace where each spectra that failed the test is masked" );

      BoundedValidator<double> *moreThanZero = new BoundedValidator<double>();
      // Variation can't be zero as we take its reciprocal, 
      // so I've set the minimum to something below which double precession arithmetic might start to fail
      moreThanZero->setLower(1e-280);
      declareProperty("Variation", 1.1, moreThanZero,
		      "Identify spectra whose total number of counts has changed by more\n"
		      "than this factor of the median change between the two input workspaces" );
      BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
      mustBePosInt->setLower(0);
      declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
		      "The index number of the first entry in the Workspace to include in\n"
		      "the calculation (default: 0)" );

      //Mantid::EMPTY_INT() and EMPTY_DBL() are tags that indicate that no 
      // value has been set and we want to use the default
      declareProperty("EndWorkspaceIndex", Mantid::EMPTY_INT(), mustBePosInt->clone(),
		      "The index number of the last entry in the Workspace to include in\n"
		      "the calculation (default: the last spectrum in the workspace)" );
      declareProperty("RangeLower", Mantid::EMPTY_DBL(),
		      "No bin with a boundary at an x value less than this will be included\n"
		      "in the summation used to decide if a detector is 'bad' (default: the\n"
		      "start of each histogram)" );
      declareProperty("RangeUpper", Mantid::EMPTY_DBL(),
		      "No bin with a boundary at an x value higher than this value will\n"
		      "be included in the summation used to decide if a detector is 'bad'\n"
		      "(default: the end of each histogram)" );
      declareProperty("NumberOfFailures", 0, Direction::Output);
    }

    /** Executes the algorithm that includes calls to SolidAngle and Integration
     *
     *  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
     *  @throw runtime_error if a sub-algorithm cannot execute
     */
    void DetectorEfficiencyVariation::exec()
    {
      MatrixWorkspace_sptr WB1;
      MatrixWorkspace_sptr WB2;
      double variation = Mantid::EMPTY_DBL();
      int minSpec = 0;
      int maxSpec = Mantid::EMPTY_INT();
      retrieveProperties( WB1, WB2, variation, minSpec, maxSpec);

      const double rangeLower = getProperty("RangeLower");
      const double rangeUpper = getProperty("RangeUpper");

      MatrixWorkspace_sptr counts1 = integrateSpectra(WB1, minSpec, maxSpec, rangeLower, 
						      rangeUpper);
      MatrixWorkspace_sptr counts2 = integrateSpectra(WB2, minSpec, maxSpec, rangeLower, 
						      rangeUpper);
      MatrixWorkspace_sptr countRatio;
      try
      {
	// Note. This can produce NAN/INFs. Leave for now and sort it out in the later tests
	countRatio = counts1/counts2;     
      } 
      catch (std::invalid_argument&) 
      {
	g_log.error() << "The two white beam workspaces size must match.";
	throw;
      }
      std::set<int> badIndices;
      double average = calculateMedian(countRatio, badIndices);
      g_log.notice() << name() << ": The median of the ratio of the integrated counts is: " 
		     << average << std::endl;
      // 
      int numFailed = doDetectorTests(counts1, counts2, average, variation, badIndices);

      g_log.information() << "Tests failed " << numFailed << " spectra. "
			  << "These have been masked on the OutputWorkspace\n";

      // counts1 was overwriten by the last function, now register it 
      setProperty("NumberOfFailures", numFailed);
    }


    /** Loads, checks and passes back the values passed to the algorithm
     * @param whiteBeam1 :: A white beam vanadium spectrum that will be used to check detector efficiency variations
     * @param whiteBeam2 :: The other white beam vanadium spectrum from the same instrument to use for comparison
     * @param variation :: The maximum fractional variation above the median that is allowed for god detectors
     * @param minSpec :: Index number of the first spectrum to use
     * @param maxSpec :: Index number of the last spectrum to use
     * @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
     */
    void DetectorEfficiencyVariation::
    retrieveProperties(API::MatrixWorkspace_sptr &whiteBeam1, API::MatrixWorkspace_sptr &whiteBeam2,
		       double &variation, int &minSpec, int &maxSpec )
    {
      whiteBeam1 = getProperty("WhiteBeamBase");
      whiteBeam2 = getProperty("WhiteBeamCompare");
      if ( whiteBeam1->getBaseInstrument()->getName() !=
	   whiteBeam2->getBaseInstrument()->getName() )
      {
	throw std::invalid_argument("The two input white beam vanadium workspaces must be from the same instrument");
      }
      int maxSpecIndex = static_cast<int>(whiteBeam1->getNumberHistograms()) - 1;
      if ( maxSpecIndex != static_cast<int>(whiteBeam2->getNumberHistograms()) - 1 )
      {//we would get a crash later on if this were not true
	throw std::invalid_argument("The input white beam vanadium workspaces must be have the same number of histograms");
      }
      // construting this object will throw an invalid_argument if there is no instrument information, 
      //we don't catch it we the algorithm will be stopped
      InputWSDetectorInfo testingTesting(whiteBeam1);
      InputWSDetectorInfo testingTestingTesting(whiteBeam2); 
      try
      {//now try to access the detector map, this is non-fatal
	testingTesting.aDetecIsMaskedinSpec(0);
	testingTestingTesting.aDetecIsMaskedinSpec(0);
      }
      catch(Kernel::Exception::NotFoundError)
      {
	m_usableMaskMap = false;
	// it still makes sense to carry on
	g_log.warning(
	  "Precision warning: Detector masking map can't be found, assuming that no detectors "
	  "have been previously marked unreliable in this workspace");
      }

      variation = getProperty("Variation");
  
      minSpec = getProperty("StartWorkspaceIndex");
      if ( (minSpec < 0) || (minSpec > maxSpecIndex) )
      {
	g_log.warning("StartWorkspaceIndex out of range, changed to 0");
	minSpec = 0;
      }
      maxSpec = getProperty("EndWorkspaceIndex");
      if (maxSpec == Mantid::EMPTY_INT()) maxSpec = maxSpecIndex;
      if ( (maxSpec < 0) || (maxSpec > maxSpecIndex ) )
      {
	g_log.warning("EndWorkspaceIndex out of range, changed to max Workspace number");
	maxSpec = maxSpecIndex;
      }
      if ( (maxSpec < minSpec) )
      {
	g_log.warning("EndWorkspaceIndex can not be less than the StartWorkspaceIndex, "
		      "changed to max Workspace number");
	maxSpec = maxSpecIndex;
      }
    }

    /**
     * Apply the detector test criterion
     * @param counts1 :: A workspace containing the integrated counts of the first white beam run
     * @param counts2 :: A workspace containing the integrated counts of the first white beam run
     * @param average :: The computed median
     * @param variation :: The allowed variation in terms of number of medians, i.e those spectra where
     * the ratio of the counts outside this range will fail the tests and will be masked on counts1
     * @param badIndices :: A set of indexs that mark bad spectra to skip in the tests
     * @return number of detectors for which tests failed
     */
    int DetectorEfficiencyVariation::doDetectorTests(API::MatrixWorkspace_const_sptr counts1, 
						     API::MatrixWorkspace_const_sptr counts2,
						     const double average, double variation,
						     const std::set<int> & badIndices)
    {
      // DIAG in libISIS did this.  A variation of less than 1 doesn't make sense in this algorithm
      if ( variation < 1 )
      {
	variation = 1.0/variation;
      }
      // criterion for if the the first spectrum is larger than expected
      double largest = average*variation;
      // criterion for if the the first spectrum is lower than expected
      double lowest = average/variation;

      const int numSpec = static_cast<int>(counts1->getNumberHistograms());
      const int progStep = static_cast<int>(ceil(numSpec/30.0));

      // Create a workspace for the output
      MatrixWorkspace_sptr maskWS = WorkspaceFactory::Instance().create(counts1); 
      // Make sure the output is simple
      maskWS->isDistribution(false);
      maskWS->setYUnit("");

      const double liveValue(1.0);
      int numFailed(0);
      PARALLEL_FOR3(counts1, counts2, maskWS)
      for (int i = 0; i < static_cast<int>(numSpec); ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        // move progress bar
        if (i % progStep == 0)
        {
          advanceProgress( progStep*static_cast<double>(RTMarkDetects)/numSpec );
          progress( m_fracDone );
          interruption_point();
        }

        // If the index is marked as bad then just give it the dead value as
        // it means that either there is no detector, it's masked already or it is NAN/INF
        if( badIndices.count(i) == 1)
        {
          maskWS->maskWorkspaceIndex(i);
          continue;
        }
        // Check the ratio is within the given range
        const double ratio = counts1->readY(i)[0]/counts2->readY(i)[0];
        if( ratio < lowest || ratio > largest )
        {
          PARALLEL_CRITICAL(detEfficVar_critical_a)
          {
            maskWS->maskWorkspaceIndex(i);
            ++numFailed;
          }
        }
        else
        {
          // Mark it with the "live" value
          maskWS->dataY(i)[0] = liveValue;
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Register the results with the ADS
      setProperty("OutputWorkspace", maskWS);
      
      return numFailed;
    }

  } // namespace Algorithm
} // namespace Mantid
