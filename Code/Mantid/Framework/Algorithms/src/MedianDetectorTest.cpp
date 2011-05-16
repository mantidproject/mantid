#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MedianDetectorTest)

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;

    /// Default constructor
    MedianDetectorTest::MedianDetectorTest() :
      DetectorDiagnostic(), m_inputWS(), m_Low(0.1), m_High(1.5), m_minSpec(0), 
      m_maxSpec(EMPTY_INT()), m_rangeLower(0.0), m_rangeUpper(0.0)
    {
    };

    /// Declare algorithm properties
    void MedianDetectorTest::init()
    {
      declareProperty(
	new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
				new HistogramValidator<>),
	"Name of the input workspace" );
      declareProperty(
	new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
	"A MaskWorkspace where 0 denotes a masked spectra. Any spectra containing"
	"a zero is also masked on the output");

      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0);
      declareProperty("SignificanceTest", 3.3, mustBePositive,
		      "Set this to a nonzero value and detectors in spectra with a total\n"
		      "number of counts is within this number of standard deviations from the\n"
		      "median will not be labelled bad (default 3.3)" );
      declareProperty("LowThreshold", 0.1,
		      "Detectors corresponding to spectra with total counts equal to or less\n"
		      "than this proportion of the median number of counts will be identified\n"
		      "as reading badly (default 0.1)" );
      declareProperty("HighThreshold", 1.5,
		      "Detectors corresponding to spectra with total counts equal to or more\n"
		      "than this number of the median will be identified as reading badly\n"
		      "(default 1.5)" );
      BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
      mustBePosInt->setLower(0);
      declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
		      "The index number of the first spectrum to include in the calculation\n"
		      "(default 0)" );
      declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt->clone(),
		      "The index number of the last spectrum to include in the calculation\n"
		      "(default the last histogram)" );
      declareProperty("RangeLower", EMPTY_DBL(),
		      "No bin with a boundary at an x value less than this will be included\n"
		      "in the summation used to decide if a detector is 'bad' (default: the\n"
		      "start of each histogram)" );
      declareProperty("RangeUpper", EMPTY_DBL(),
		      "No bin with a boundary at an x value higher than this value will\n"
		      "be included in the summation used to decide if a detector is 'bad'\n"
		      "(default: the end of each histogram)" );
      declareProperty("NumberOfFailures", 0, Direction::Output);
    }

    /** Executes the algorithm that includes calls to SolidAngle and Integration
     *
     *  @throw invalid_argument if there is an incapatible property value and so the algorithm can't continue
     *  @throw runtime_error if algorithm cannot execute
     */
    void MedianDetectorTest::exec()
    {
      retrieveProperties();

      

      //Adds the counts from all the bins and puts them in one total bin
      MatrixWorkspace_sptr counts = integrateSpectra(m_inputWS, m_minSpec, m_maxSpec, 
						     m_rangeLower, m_rangeUpper, true);
      counts = convertToRate(counts); 

	  // FIXME: The next section that calculates the solid angle is commented out until 
	  //        the SolidAngle algorithm is corrected to return the correct number.
	  //        (see http://trac.mantidproject.org/mantid/ticket/2596)
	  
      //MatrixWorkspace_sptr angles = getSolidAngles(m_minSpec, m_maxSpec);

      //Gets the count rate per solid angle (in steradians), if it exists, for each spectrum
      //this calculation is optional, it depends on angle information existing
      //if ( angles.use_count() == 1 )
      //{
		//if some numbers in angles are zero we will get the infinity flag value 
		// in the output work space which needs to be dealt with later
	  //counts = counts/angles;     
      //}
      
      // End of Solid Angle commented out section
	   

      // An average of the data, the median is less influenced by a small number of huge values than the mean
      std::set<int> badIndices;
      double average = calculateMedian(counts, badIndices);

      // Create a workspace for the output
      MatrixWorkspace_sptr maskWS = WorkspaceFactory::Instance().create(counts); 
      // Make sure the output is simple
      maskWS->isDistribution(false);
      maskWS->setYUnit("");
      maskWS->instrumentParameters().clear();

      int numFailed = doDetectorTests(counts, maskWS, average, badIndices);

      g_log.information() << "Median test results:\n"
			  << "\tNumber of failures - " << numFailed << "\n"
			  << "\tNumber of skipped spectra - " << badIndices.size() << "\n";

      setProperty("NumberOfFailures", numFailed);
      setProperty("OutputWorkspace", maskWS);
    }

    /** Loads and checks the values passed to the algorithm
     *
     *  @throw invalid_argument if there is an incompatible property value so the algorithm can't continue
     */
    void MedianDetectorTest::retrieveProperties()
    {
      m_inputWS = getProperty("InputWorkspace");
      size_t maxSpecIndex = m_inputWS->getNumberHistograms() - 1;

      m_minSpec = getProperty("StartWorkspaceIndex");
      if ( (m_minSpec < 0) || (m_minSpec > maxSpecIndex) )
      {
	g_log.warning("StartSpectrum out of range, changed to 0");
	m_minSpec = 0;
      }
      m_maxSpec = getProperty("EndWorkspaceIndex");
      if (m_maxSpec == EMPTY_INT() ) m_maxSpec = maxSpecIndex;
      if ( (m_maxSpec < 0) || (m_maxSpec > maxSpecIndex ) )
      {
	g_log.warning("EndSpectrum out of range, changed to max spectrum number");
	m_maxSpec = maxSpecIndex;
      }
      if ( (m_maxSpec < m_minSpec) )
      {
	g_log.warning("EndSpectrum can not be less than the StartSpectrum, changed to max spectrum number");
	m_maxSpec = maxSpecIndex;
      }

      m_Low = getProperty("LowThreshold");
      m_High = getProperty("HighThreshold");
      if ( !(m_Low < m_High) )
      {
	throw std::invalid_argument("The threshold for reading high must be greater than the low threshold");
      }

      // Integration Range
      m_rangeLower = getProperty("RangeLower");
      m_rangeUpper = getProperty("RangeUpper");
    }

    /** Makes a worksapce with the total solid angle all the detectors in each spectrum cover from the sample
     *  note returns an empty shared pointer on failure, uses the SolidAngle algorithm
     * @param firstSpec :: the index number of the first histogram to analyse
     * @param lastSpec :: the index number of the last histogram to analyse
     * @return A pointer to the workspace (or an empty pointer)
     */
    API::MatrixWorkspace_sptr MedianDetectorTest::getSolidAngles(int firstSpec, int lastSpec )
    {
      g_log.information("Calculating soild angles");
      // get percentage completed estimates for now, t0 and when we've finished t1
      double t0 = m_fracDone, t1 = advanceProgress(RTGetSolidAngle);
      IAlgorithm_sptr childAlg = createSubAlgorithm("SolidAngle", t0, t1);
      childAlg->setProperty( "InputWorkspace", m_inputWS);
      childAlg->setProperty( "StartWorkspaceIndex", firstSpec );
      childAlg->setProperty( "EndWorkspaceIndex", lastSpec );
      try
      {
        // Execute the sub-algorithm, it could throw a runtime_error at this point which would abort execution
        childAlg->execute();
        if ( ! childAlg->isExecuted() )
        {
          throw std::runtime_error("Unexpected problem calculating solid angles");
        }
      }
      //catch all exceptions because the solid angle calculation is optional
      catch(std::exception e)
      {
        g_log.warning(
            "Precision warning:  Can't find detector geometry " + name() +
            " will continue with the solid angles of all spectra set to the same value" );
        failProgress(RTGetSolidAngle);
        //The return is an empty workspace pointer, which must be handled by the calling function
        MatrixWorkspace_sptr empty;
        //function returns normally
        return empty;
      }
      return childAlg->getProperty("OutputWorkspace");
    }

    /** 
     * Takes a single valued histogram workspace and assesses which histograms are within the limits. 
     * Those that are not are masked on the input workspace.
     * @param countWorkspace :: Input/Output Integrated workspace to diagnose
     * @param average :: The expected number of counts, spectra within defined threshold won't fail
     * @param badIndices :: If an index is in this list then it will not be included in the tests
     * @return The number of detectors that failed the tests, not including those skipped
     */
    int MedianDetectorTest::doDetectorTests(const API::MatrixWorkspace_sptr countWorkspace,
					    API::MatrixWorkspace_sptr maskWS, 
					    const double average, const std::set<int> & badIndices)
    {
      g_log.information("Applying the criteria to find failing detectors");
  
      const double lowLim = (average*m_Low);
      const double highLim = (average*m_High);
      // A spectra can't fail if the statistics show its value is consistent with the mean value, 
      // check the error and how many errorbars we are away
      const double minSigma = getProperty("SignificanceTest");

      // prepare to report progress
      const int numSpec(m_maxSpec - m_minSpec);
      const int progStep = static_cast<int>(ceil(numSpec/30.0));

      const double live_value(1.0);
      int numLow(0), numHigh(0);
      
      //set the workspace to have no units
      countWorkspace->isDistribution(false);
      countWorkspace->setYUnit("");

      PARALLEL_FOR1(countWorkspace)
      for (int i = 0; i <= numSpec; ++i)
      {
	PARALLEL_START_INTERUPT_REGION
	  
	// update the progressbar information
	if (i % progStep == 0)
	{
	  progress(advanceProgress(progStep*static_cast<double>(RTMarkDetects)/numSpec));
	}


	// If the value is not in the badIndices set then assume it is good
	// else skip tests for it
	if( badIndices.count(i) == 1 )
	{
	  maskWS->maskWorkspaceIndex(i);
	  continue;
	}
	//Do tests
	const double sig = minSigma*countWorkspace->readE(i)[0];
	// Check the significance value is okay
	if( boost::math::isinf(std::abs(sig)) || boost::math::isinf(sig) )
	{
	  PARALLEL_CRITICAL(MedianDetectorTest_failed_a)
	  {
	    maskWS->maskWorkspaceIndex(i);
	    ++numLow;
	  }
	  continue;
	}

	const double yIn = countWorkspace->dataY(i)[0];
	if ( yIn <= lowLim )
	{
	  // compare the difference against the size of the errorbar -statistical significance check
	  if(average - yIn > sig)
	  {
	    PARALLEL_CRITICAL(MedianDetectorTest_failed_b)
	    {
	      maskWS->maskWorkspaceIndex(i);
	      ++numLow;
	    }
	    continue;
	  }
	}
	if (yIn >= highLim)
	{
	  // compare the difference against the size of the errorbar -statistical significance check
	  if(yIn - average > sig)
	  {
	    PARALLEL_CRITICAL(MedianDetectorTest_failed_c)
	    {
	      maskWS->maskWorkspaceIndex(i);
	      ++numHigh;
	    }
	    continue;
	  }
	}
	// Reaching here passes the tests
	maskWS->dataY(i)[0] = live_value;
	
	PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Log finds
      g_log.information() << "-- Detector tests --\n " 
			  << "Number recording low: " << numLow << "\n"
			  << "Number recording high: " << numHigh << "\n";
      
      return (numLow + numHigh);
    }


  } // namespace Algorithm
} // namespace Mantid
