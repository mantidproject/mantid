/*WIKI* 


If instrument geometry information is available the [[SolidAngle]] algorithm is used to calculate the number of counts per unit solid angle, otherwise numbers of counts are used without correction.  First the median number of counts in all the bins is calculated.  Then the ratio of the total number of counts and the median number is calculated for each histogram. This ratio is compared against the user defined upper and lower thresholds and if the ratio is outside the limits the statistical significance test is done.

In the statistical significance test the difference between the number of counts in each spectrum and the median number is compared to the spectrum's error value.  Any spectra where the ratio of the its deviation from the mean and the its error is less than the value of the property SignificanceTest will not be labelled bad.  This test is particularly important when the number of counts is low, for example when examining the low count "background" parts of spectra.

The output workspace contains a MaskWorkspace where those spectra that fail the tests are masked and those that pass them are assigned a single positive value.

===Subalgorithms used===

Uses the [[SolidAngle]], [[Integration]] and [[ConvertToDistribution]] algorithms.


*WIKI*/
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
      DetectorDiagnostic(), m_inputWS(), m_loFrac(0.1), m_hiFrac(1.5), m_minSpec(0),
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
                      "Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                      "difference with respect to the median value must also exceed this number of error bars");
      declareProperty("LowThreshold", 0.1,
                      "Lower acceptable bound as fraction of median value" );
      declareProperty("HighThreshold", 1.5,
                      "Upper acceptable bound as fraction of median value");
      declareProperty("LowOutlier", 0.01, "Lower bound defining outliers as fraction of median value");
      declareProperty("HighOutlier", 100., "Upper bound defining outliers as fraction of median value");
      declareProperty("ExcludeZeroesFromMedian", false, "If false (default) zeroes will be included in "
                      "the median calculation, otherwise they will not be included but they will be left unmasked");
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
     *  @throw invalid_argument if there is an incompatible property value and so the algorithm can't continue
     *  @throw runtime_error if algorithm cannot execute
     */
    void MedianDetectorTest::exec()
    {
      retrieveProperties();
      // Ensures we have a workspace with a single bin. It will contain any input masking and will be used to record any
      // required masking from this algorithm
      MatrixWorkspace_sptr countsWS = integrateSpectra(m_inputWS, m_minSpec, m_maxSpec,
                                                       m_rangeLower, m_rangeUpper, true);

      // Make sure the output is simple
      countsWS->setYUnit("");
      countsWS->instrumentParameters();

      // 1. Calculate the median
      const bool excludeZeroes = getProperty("ExcludeZeroesFromMedian");
      double median = calculateMedian(countsWS, excludeZeroes);
      g_log.information() << "Median value = " << median << "\n";
      // 2. Mask outliers
      int numFailed = maskOutliers(median, countsWS);
      // 3. Recalulate the median
      median = calculateMedian(countsWS, excludeZeroes);
      g_log.information() << "Median value with outliers removed = " << median << "\n";

      numFailed  += doDetectorTests(countsWS, median);
      g_log.information() << "Median test results:\n"
                       << "\tNumber of failures - " << numFailed << "\n";

      setProperty("NumberOfFailures", numFailed);

      // extract and set the mask result
      IAlgorithm_sptr childAlg = createSubAlgorithm("ExtractMask");
      childAlg->setProperty( "InputWorkspace", countsWS );
      childAlg->executeAsSubAlg();
      MatrixWorkspace_sptr maskWS = childAlg->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace", maskWS);
    }

    /** Loads and checks the values passed to the algorithm
     *
     *  @throw invalid_argument if there is an incompatible property value so the algorithm can't continue
     */
    void MedianDetectorTest::retrieveProperties()
    {
      m_inputWS = getProperty("InputWorkspace");
      int maxSpecIndex = static_cast<int>(m_inputWS->getNumberHistograms()) - 1;

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

      m_loFrac = getProperty("LowThreshold");
      m_hiFrac = getProperty("HighThreshold");
      if ( m_loFrac > m_hiFrac )
      {
        throw std::invalid_argument("The threshold for reading high must be greater than the low threshold");
      }

      // Integration Range
      m_rangeLower = getProperty("RangeLower");
      m_rangeUpper = getProperty("RangeUpper");
    }

    /** Makes a workspace with the total solid angle all the detectors in each spectrum cover from the sample
     *  note returns an empty shared pointer on failure, uses the SolidAngle algorithm
     * @param firstSpec :: the index number of the first histogram to analyse
     * @param lastSpec :: the index number of the last histogram to analyse
     * @return A pointer to the workspace (or an empty pointer)
     */
    API::MatrixWorkspace_sptr MedianDetectorTest::getSolidAngles(int firstSpec, int lastSpec )
    {
      g_log.debug("Calculating solid angles");
      // get percentage completed estimates for now, t0 and when we've finished t1
      double t0 = m_fracDone, t1 = advanceProgress(RTGetSolidAngle);
      IAlgorithm_sptr childAlg = createSubAlgorithm("SolidAngle", t0, t1, true);
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
      catch(std::exception&)
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
     * Mask the outlier values to get a better median value.
     * @param median The median value calculated from the current counts
     * @param countsWS The counts workspace. Any outliers will be masked here
     * @returns The number masked
     */
    int MedianDetectorTest::maskOutliers(const double median, API::MatrixWorkspace_sptr countsWS)
    {
      // Fractions of the median
      const double out_lo = getProperty("LowOutlier");
      const double out_hi = getProperty("HighOutlier");
      const int64_t nhist = static_cast<int64_t>(countsWS->getNumberHistograms());
      int numFailed(0);

      PARALLEL_FOR1(countsWS)
      for(int64_t i = 0; i < nhist; ++i)
      {
        const double value = countsWS->readY(i)[0];
        if( (value < out_lo*median) && (value > 0.0) )
        {
          countsWS->maskWorkspaceIndex(i);
          PARALLEL_ATOMIC
          ++numFailed;
        }
        else if( value > out_hi*median )
        {
          countsWS->maskWorkspaceIndex(i);
          PARALLEL_ATOMIC
          ++numFailed;
        }
      }
      return numFailed;
    }


    /** 
     * Takes a single valued histogram workspace and assesses which histograms are within the limits. 
     * Those that are not are masked on the input workspace.
     * @param countWorkspace :: Input/Output Integrated workspace to diagnose
     * @param maskWS :: A mask workspace to apply
     * @param average :: The expected number of counts, spectra within defined threshold won't fail
     * @param badIndices :: If an index is in this list then it will not be included in the tests
     * @return The number of detectors that failed the tests, not including those skipped
     */
    int MedianDetectorTest::doDetectorTests(const API::MatrixWorkspace_sptr countsWS,
                                            const double median)
    {
      g_log.debug("Applying the criteria to find failing detectors");
      // A spectra can't fail if the statistics show its value is consistent with the mean value, 
      // check the error and how many errorbars we are away
      const double minSigma = getProperty("SignificanceTest");

      // prepare to report progress
      const int numSpec(m_maxSpec - m_minSpec);
      const int progStep = static_cast<int>(ceil(numSpec/30.0));

      const double live_value(1.0);
      int numFailed(0);
      
      PARALLEL_FOR1(countsWS)
      for (int i = 0; i <= numSpec; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
          
        // update the progressbar information
        if (i % progStep == 0)
        {
          progress(advanceProgress(progStep*static_cast<double>(RTMarkDetects)/numSpec));
        }

        IDetector_const_sptr det;
        try
        {
          det = countsWS->getDetector(i);
        }
        catch(Exception::NotFoundError &)
        {}
        // Mark non-existant detectors as dead
        if( !det )
        {
          countsWS->maskWorkspaceIndex(i);
          continue;
        }
        if( det->isMasked() )
        {
          countsWS->dataY(i)[0] = 0.0;
          continue;
        }
        if( det->isMonitor() )
        {
          // Don't include in calculation but don't mask it
          countsWS->dataY(i)[0] = live_value;
        }

        const double signal = countsWS->dataY(i)[0];
        // Mask out NaN and infinite
        if( boost::math::isinf(signal) || boost::math::isnan(signal) )
        {
          countsWS->maskWorkspaceIndex(i);
          PARALLEL_ATOMIC
          ++numFailed;
          continue;
        }

        const double error = minSigma*countsWS->readE(i)[0];

        if( (signal < median*m_loFrac && (signal-median < -error)) ||
            (signal > median*m_hiFrac && (signal-median > error)) )
        {
          countsWS->maskWorkspaceIndex(i);
          PARALLEL_ATOMIC
          ++numFailed;
        }
        else
        {
          // Reaching here passes the tests
          countsWS->dataY(i)[0] = live_value;
        }

        
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Log finds
      g_log.information() << numFailed << " spectra failed the median tests.\n";
      
      return numFailed;
    }


  } // namespace Algorithm
} // namespace Mantid
