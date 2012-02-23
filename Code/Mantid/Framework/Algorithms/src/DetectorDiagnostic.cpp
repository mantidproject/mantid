//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/DetectorDiagnostic.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <gsl/gsl_statistics.h>
#include <cfloat>

namespace Mantid
{

  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(DetectorDiagnostic)

    using API::MatrixWorkspace_sptr;
    using API::IAlgorithm_sptr;
    using Geometry::IDetector_const_sptr;
    using std::string;
    using namespace Mantid::DataObjects;
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    //--------------------------------------------------------------------------
    // Functions to make this a proper workflow algorithm
    //--------------------------------------------------------------------------
    const std::string DetectorDiagnostic::category() const
    {
      return "Diagnostics";
    }

    const std::string DetectorDiagnostic::name() const
    {
      return "DetectorDiagnostic";
    }

    int DetectorDiagnostic::version() const
    {
      return 1;
    }

    void DetectorDiagnostic::initDocs()
    {
      this->setWikiSummary("Identifies histograms and their detectors that have total numbers of counts over a user defined maximum or less than the user define minimum. ");
      this->setOptionalMessage("Identifies histograms and their detectors that have total numbers of counts over a user defined maximum or less than the user define minimum.");
    }

    void DetectorDiagnostic::init()
    {
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
                      "Name of the input workspace" );
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","", Direction::Output),
                      "A SpecialWorkspace2D containing the masked spectra as zeroes and ones.");
      BoundedValidator<int> *mustBePosInt = new BoundedValidator<int>();
      mustBePosInt->setLower(0);
      declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
                      "The index number of the first spectrum to include in the calculation\n"
                      "(default 0)" );
      declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt->clone(),
                      "The index number of the last spectrum to include in the calculation\n"
                      "(default the last histogram)" );
      declareProperty("RangeLower", EMPTY_DBL(),
          "No bin with a boundary at an x value less than this will be used\n"
          "in the summation that decides if a detector is 'bad' (default: the\n"
          "start of each histogram)" );
      declareProperty("RangeUpper", EMPTY_DBL(),
          "No bin with a boundary at an x value higher than this value will\n"
          "be used in the summation that decides if a detector is 'bad'\n"
          "(default: the end of each histogram)" );

      string findDetOutLimGrp("Find Detectors Outside Limits");
      declareProperty("LowThreshold", 0.0,
          "Spectra whose total number of counts are equal to or below this value\n"
          "will be marked bad (default 0)" );
      this->setPropertyGroup("LowThreshold", findDetOutLimGrp);
      declareProperty("HighThreshold", EMPTY_DBL(),
          "Spectra whose total number of counts are equal to or above this value\n"
          "will be marked bad (default off)" );
      this->setPropertyGroup("HighThreshold", findDetOutLimGrp);

      string medianDetTestGrp("Median Detector Test");
      declareProperty("RunMedianDetectorTest", true, "");
      this->setPropertyGroup("RunMedianDetectorTest", medianDetTestGrp);
      BoundedValidator<double> *mustBePositiveDbl = new BoundedValidator<double>();
      mustBePositiveDbl->setLower(0);
      declareProperty("SignificanceTest", 3.3, mustBePositiveDbl,
                      "Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                      "difference with respect to the median value must also exceed this number of error bars");
      this->setPropertyGroup("SignificanceTest", medianDetTestGrp);
      declareProperty("LowThresholdFraction", 0.1,
                      "Lower acceptable bound as fraction of median value" );
      this->setPropertyGroup("LowThresholdFraction", medianDetTestGrp);
      declareProperty("HighThresholdFraction", 1.5,
                      "Upper acceptable bound as fraction of median value");
      this->setPropertyGroup("HighThresholdFraction", medianDetTestGrp);
      declareProperty("LowOutlier", 0.01, "Lower bound defining outliers as fraction of median value");
      this->setPropertyGroup("LowOutlier", medianDetTestGrp);
      declareProperty("HighOutlier", 100., "Upper bound defining outliers as fraction of median value");
      this->setPropertyGroup("HighOutlier", medianDetTestGrp);
      declareProperty("ExcludeZeroesFromMedian", false, "If false (default) zeroes will be included in "
                      "the median calculation, otherwise they will not be included but they will be left unmasked");
      this->setPropertyGroup("ExcludeZeroesFromMedian", medianDetTestGrp);
      setPropertySettings("SignificanceTest", new EnabledWhenProperty(this, "RunMedianDetectorTest", IS_EQUAL_TO, "1"));
      setPropertySettings("LowThresholdFraction", new EnabledWhenProperty(this, "RunMedianDetectorTest", IS_EQUAL_TO, "1"));
      setPropertySettings("HighThresholdFraction", new EnabledWhenProperty(this, "RunMedianDetectorTest", IS_EQUAL_TO, "1"));
      setPropertySettings("LowOutlier", new EnabledWhenProperty(this, "RunMedianDetectorTest", IS_EQUAL_TO, "1"));
      setPropertySettings("HighOutlier", new EnabledWhenProperty(this, "RunMedianDetectorTest", IS_EQUAL_TO, "1"));
      setPropertySettings("ExcludeZeroesFromMedian", new EnabledWhenProperty(this, "RunMedianDetectorTest", IS_EQUAL_TO, "1"));

      string detEffVarGrp("Detector Efficiency Variation");
      declareProperty(new WorkspaceProperty<>("WhiteBeamCompare","",Direction::Input, true),
                      "Name of a matching second white beam vanadium run from the same\n"
                      "instrument" );
      this->setPropertyGroup("WhiteBeamCompare", detEffVarGrp);
      declareProperty("WhiteBeamVariation", 1.1, mustBePositiveDbl->clone(),
                      "Identify spectra whose total number of counts has changed by more\n"
                      "than this factor of the median change between the two input workspaces" );
      this->setPropertyGroup("WhiteBeamVariation", detEffVarGrp);
      setPropertySettings("WhiteBeamVariation", new EnabledWhenProperty(this, "WhiteBeamCompare", IS_NOT_DEFAULT));

      string psdBleedMaskGrp("Create PSD Bleed Mask");
      declareProperty("MaxTubeFramerate", 0.0, mustBePositiveDbl->clone(),
          "The maximum rate allowed for a tube in counts/us/frame.");
      this->setPropertyGroup("MaxTubeFramerate", psdBleedMaskGrp);
      declareProperty("NIgnoredCentralPixels", 80, mustBePosInt->clone(),
          "The number of pixels about the centre to ignore.");
      this->setPropertyGroup("NIgnoredCentralPixels", psdBleedMaskGrp);
      setPropertySettings("NIgnoredCentralPixels", new EnabledWhenProperty(this, "MaxTubeFramerate", IS_NOT_DEFAULT));

      declareProperty("NumberOfFailures", 0, Direction::Output);
    }

    void DetectorDiagnostic::exec()
    {
      // get the generic information that everybody uses
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      int minIndex = getProperty("StartWorkspaceIndex");
      int maxIndex = getProperty("EndWorkspaceIndex");
      const double rangeLower = getProperty("RangeLower");
      const double rangeUpper = getProperty("RangeUpper");

      // integrate the data once to pass to subalgorithms
      m_fracDone = 0.;
      MatrixWorkspace_sptr countsWS = integrateSpectra(inputWS, minIndex, maxIndex,
                                                       rangeLower, rangeUpper, true);

      int numFailed(0);
      MatrixWorkspace_sptr maskWS;

      // FindDetectorsOutsideLimits (always)
      {
        // get the relavant inputs
        double lowThreshold = getProperty("LowThreshold");
        double highThreshold = getProperty("HighThreshold");

        // run the subalgorithm
        IAlgorithm_sptr alg = createSubAlgorithm("FindDetectorsOutsideLimits"); // should set progress bar
        alg->setProperty("InputWorkspace", countsWS);
        alg->setProperty("StartWorkspaceIndex", minIndex);
        alg->setProperty("EndWorkspaceIndex", maxIndex);
        alg->setProperty("RangeLower", rangeLower);
        alg->setProperty("RangeUpper", rangeUpper);
        alg->setProperty("LowThreshold", lowThreshold);
        alg->setProperty("HighThreshold", highThreshold);
        alg->executeAsSubAlg();
        maskWS = alg->getProperty("OutputWorkspace");
        int localFails = alg->getProperty("NumberOfFailures");
        numFailed += localFails;
      }

      // MedianDetectorTest (if selected)
      if (getProperty("RunMedianDetectorTest"))
      {
        // apply mask to what we are going to input
        applyMask(countsWS, maskWS);

        // get the relavant inputs
        double significanceTest = getProperty("SignificanceTest");
        double lowThreshold = getProperty("LowThresholdFraction");
        double highThreshold = getProperty("HighThresholdFraction");
        double lowOutlier = getProperty("LowOutlier");
        double highOutlier = getProperty("HighOutlier");
        bool excludeZeroes = getProperty("ExcludeZeroesFromMedian");

        // run the subalgorithm
        IAlgorithm_sptr alg = createSubAlgorithm("MedianDetectorTest"); // should set progress bar
        alg->setProperty("InputWorkspace", countsWS);
        alg->setProperty("StartWorkspaceIndex", minIndex);
        alg->setProperty("EndWorkspaceIndex", maxIndex);
        alg->setProperty("RangeLower", rangeLower);
        alg->setProperty("RangeUpper", rangeUpper);
        alg->setProperty("SignificanceTest", significanceTest);
        alg->setProperty("LowThreshold", lowThreshold);
        alg->setProperty("HighThreshold", highThreshold);
        alg->setProperty("LowOutlier", lowOutlier);
        alg->setProperty("HighOutlier", highOutlier);
        alg->setProperty("ExcludeZeroesFromMedian", excludeZeroes);
        alg->executeAsSubAlg();
        MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
        maskWS += localMaskWS;
        int localFails = alg->getProperty("NumberOfFailures");
        numFailed += localFails;
      }

      // DetectorEfficiencyVariation (only if two workspaces are specified)
      if (!getPropertyValue("WhiteBeamCompare").empty())
      {
        // apply mask to what we are going to input
        applyMask(countsWS, maskWS);

        // get the relavant inputs
        MatrixWorkspace_sptr compareWS = getProperty("WhiteBeamCompare");
        double variation = getProperty("WhiteBeamVariation");

        // run the subalgorithm
        IAlgorithm_sptr alg = createSubAlgorithm("DetectorEfficiencyVariation"); // should set progress bar
        alg->setProperty("WhiteBeamBase", countsWS);
        alg->setProperty("WhiteBeamCompare", compareWS);
        alg->setProperty("StartWorkspaceIndex", minIndex);
        alg->setProperty("EndWorkspaceIndex", maxIndex);
        alg->setProperty("RangeLower", rangeLower);
        alg->setProperty("RangeUpper", rangeUpper);
        alg->setProperty("Variation", variation);
        alg->executeAsSubAlg();
        MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
        maskWS += localMaskWS;
        int localFails = alg->getProperty("NumberOfFailures");
        numFailed += localFails;
      }

      // CreatePSDBleedMask (if selected)
      double maxTubeFrameRate = getProperty("MaxTubeFramerate");
      g_log.notice() << "MaxTubeFramerate \"" << maxTubeFrameRate << "\"\n";
      if (maxTubeFrameRate > 0.)
      {
        // apply mask to what we are going to input
        applyMask(countsWS, maskWS);

        // get the relavant inputs
        int numIgnore = getProperty("NIgnoredCentralPixels");

        // run the subalgorithm
        IAlgorithm_sptr alg = createSubAlgorithm("CreatePSDBleedMask"); // should set progress bar
        alg->setProperty("InputWorkspace", inputWS);
        alg->setProperty("MaxTubeFramerate", maxTubeFrameRate);
        alg->setProperty("NIgnoredCentralPixels", numIgnore);
        alg->executeAsSubAlg();
        MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
        maskWS += localMaskWS;
        int localFails = alg->getProperty("NumberOfFailures");
        numFailed += localFails;
      }

      g_log.information() << numFailed << " spectra are being masked\n";
      setProperty("NumberOfFailures", numFailed);
      setProperty("OutputWorkspace", maskWS);
    }

    void DetectorDiagnostic::applyMask(API::MatrixWorkspace_sptr inputWS,
                                       API::MatrixWorkspace_sptr maskWS)
    {
      IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors"); // should set progress bar
      maskAlg->setProperty("Workspace", inputWS);
      maskAlg->setProperty("MaskedWorkspace", maskWS);
      maskAlg->executeAsSubAlg();
    }

    //--------------------------------------------------------------------------
    // Public member functions
    //--------------------------------------------------------------------------
    DetectorDiagnostic::DetectorDiagnostic() : 
      API::Algorithm(), m_fracDone(0.0), m_TotalTime(RTTotal)
    {
    }

    //--------------------------------------------------------------------------
    // Protected member functions
    //--------------------------------------------------------------------------

    /**
     * Integrate each spectra to get the number of counts
     * @param inputWS :: The workspace to integrate
     * @param indexMin :: The lower bound of the spectra to integrate
     * @param indexMax :: The upper bound of the spectra to integrate
     * @param lower :: The lower bound
     * @param upper :: The upper bound
     * @param outputWorkspace2D :: set to true to output a workspace 2D even if the input is an EventWorkspace
     * @returns A workspace containing the integrated counts
     */
    MatrixWorkspace_sptr 
    DetectorDiagnostic::integrateSpectra(MatrixWorkspace_sptr inputWS, 
        const int indexMin, const int indexMax, const double lower, const double upper,
        const bool outputWorkspace2D)
    {
      g_log.debug() << "Integrating input spectra.\n";
      // If the input spectra only has one bin, assume it has been integrated already
      // but we need to pass it to the algorithm so that a copy of the input workspace is
      // actually created to use for further calculations
      // get percentage completed estimates for now, t0 and when we've finished t1
      double t0 = m_fracDone, t1 = advanceProgress(RTGetTotalCounts);
      IAlgorithm_sptr childAlg = createSubAlgorithm("Integration", t0, t1 );
      childAlg->setProperty( "InputWorkspace", inputWS );
      childAlg->setProperty( "StartWorkspaceIndex", indexMin );
      childAlg->setProperty( "EndWorkspaceIndex", indexMax );
      // pass inputed values straight to this integration trusting the checking done there
      childAlg->setProperty("RangeLower",  lower );
      childAlg->setProperty("RangeUpper", upper);
      childAlg->setPropertyValue("IncludePartialBins", "1");
      childAlg->executeAsSubAlg();

      // Convert to 2D if desired, and if the input was an EventWorkspace.
      MatrixWorkspace_sptr outputW = childAlg->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr finalOutputW = outputW;
      if (outputWorkspace2D && boost::dynamic_pointer_cast<EventWorkspace>(outputW))
      {
        g_log.debug() << "Converting output Event Workspace into a Workspace2D." << std::endl;
        childAlg = createSubAlgorithm("ConvertToMatrixWorkspace", t0, t1 );
        childAlg->setProperty("InputWorkspace", outputW);
        childAlg->executeAsSubAlg();
        finalOutputW = childAlg->getProperty("OutputWorkspace");
      }

      return finalOutputW;
    }

    /**
     * Create a masking workspace to return.
     *
     * @param inputWS The workspace to initialize from. The instrument is copied from this.
     * @param initialize Whether or not to set all of the values to keep the data.
     */
    MatrixWorkspace_sptr DetectorDiagnostic::generateEmptyMask(API::MatrixWorkspace_const_sptr inputWS, const bool initialize)
    {
      // Create a new workspace for the results, copy from the input to ensure that we copy over the instrument and current masking
      DataObjects::SpecialWorkspace2D* maskWS = new DataObjects::SpecialWorkspace2D();
      maskWS->initialize(inputWS->getNumberHistograms(), 1, 1);
      MatrixWorkspace_sptr outputWS(maskWS);
      WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
      outputWS->setTitle(inputWS->getTitle());
      outputWS->getInstrument()->getParameterMap()->clearParametersByName("masked"); // turn off the mask bit

      if (initialize) // initialize the mask as keep everything
      {
        // initialize the mask as keep everything
        const double liveValue(0.0); // keep the data - delete is 1.0
        const double errors(0.0); // uncertainty
        int64_t size = outputWS->getNumberHistograms();
        PARALLEL_FOR1(outputWS)
        for (int64_t i = 0; i < size; i++)
        {
          PARALLEL_START_INTERUPT_REGION
          outputWS->dataY(i)[0] = liveValue; // by default it stays
          outputWS->dataE(i)[0] = errors;
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }

      return outputWS;
    }

    /** 
     *  Fnds the median of values in single bin histograms rejecting spectra from masked
     *  detectors and the results of divide by zero (infinite and NaN).  
     * The median is an average that is less affected by small numbers of very large values.
     * @param input :: A histogram workspace with one entry in each bin
     * @param excludeZeroes :: If true then zeroes will not be included in the median calculation
     * @return The median value of the histograms in the workspace that was passed to it
     * @throw out_of_range if a value is negative
     */
    double DetectorDiagnostic::calculateMedian(const API::MatrixWorkspace_sptr input, bool excludeZeroes)
    {
      g_log.debug("Calculating the median count rate of the spectra");

      std::vector<double> medianInput;
      const int nhists = static_cast<int>(input->getNumberHistograms());
      // The maximum possible length is that of workspace length
      medianInput.reserve(nhists);

      bool checkForMask = false;
      Geometry::Instrument_const_sptr instrument = input->getInstrument();
      if (instrument != NULL)
      {
        checkForMask = ((instrument->getSource() != NULL) && (instrument->getSample() != NULL));
      }

      PARALLEL_FOR1(input)
      for (int i = 0; i < nhists; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        if (checkForMask) {
          const std::set<detid_t>& detids = input->getSpectrum(i)->getDetectorIDs();
          if (instrument->isDetectorMasked(detids))
            continue;
          if (instrument->isMonitor(detids))
            continue;
        }

        const double yValue = input->readY(i)[0];
        if ( yValue  < 0.0 )
        {
          throw std::out_of_range("Negative number of counts found, could be corrupted raw counts or solid angle data");
        }
        if( boost::math::isnan(yValue) || boost::math::isinf(yValue) ||
            (excludeZeroes && yValue < DBL_EPSILON)) // NaNs/Infs
        {
          continue;
        }
        // Now we have a good value
        PARALLEL_CRITICAL(DetectorDiagnostic_median_d)
        {
          medianInput.push_back(yValue);
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      if(medianInput.empty()){
          throw std::out_of_range(" no single valid histohrams identified in the workspace");
      }

      // We need a sorted array to calculate the median
      std::sort(medianInput.begin(), medianInput.end());
      double median = gsl_stats_median_from_sorted_data( &medianInput[0], 1, medianInput.size() );

      if ( median < 0 || median > DBL_MAX/10.0 )
      {
        throw std::out_of_range("The calculated value for the median was either negative or unreliably large");
      }
      return median;
    }

    /** 
     * Convert to a distribution
     * @param workspace :: The input workspace to convert to a count rate
     * @return distribution workspace with equiv. data
     */
    API::MatrixWorkspace_sptr DetectorDiagnostic::convertToRate(API::MatrixWorkspace_sptr workspace)
    {
      if( workspace->isDistribution() )
      {
        g_log.information() << "Workspace already contains a count rate, nothing to do.\n";
        return workspace;
      }

      g_log.information("Calculating time averaged count rates");
      // get percentage completed estimates for now, t0 and when we've finished t1
      double t0 = m_fracDone, t1 = advanceProgress(RTGetRate);
      IAlgorithm_sptr childAlg = createSubAlgorithm("ConvertToDistribution", t0, t1);
      childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", workspace); 
      // Now execute the sub-algorithm but allow any exception to bubble up
      childAlg->execute();
      return childAlg->getProperty("Workspace");
    }



    /** Update the percentage complete estimate assuming that the algorithm 
     * has completed a task with the given estimated run time
     * @param toAdd :: the estimated additional run time passed since the last update, 
     * where m_TotalTime holds the total algorithm run time
     * @return estimated fraction of algorithm runtime that has passed so far
     */
    double DetectorDiagnostic::advanceProgress(double toAdd)
    {
      m_fracDone += toAdd/m_TotalTime;
      // it could go negative as sometimes the percentage is re-estimated backwards, 
      // this is worrying about if a small negative value will cause a problem some where
      m_fracDone = std::abs(m_fracDone);
      interruption_point();
      return m_fracDone;
    }

    /** Update the percentage complete estimate assuming that the algorithm aborted a task with the given
     *  estimated run time
     * @param aborted :: the amount of algorithm run time that was saved by aborting a 
     * part of the algorithm, where m_TotalTime holds the total algorithm run time
     */
    void DetectorDiagnostic::failProgress(RunTime aborted)
    {
      advanceProgress(-aborted);
      m_TotalTime -= aborted;
    };



  }

}
