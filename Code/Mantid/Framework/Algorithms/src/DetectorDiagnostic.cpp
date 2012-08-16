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
#include "MantidDataObjects/MaskWorkspace.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <gsl/gsl_statistics.h>
#include <cfloat>
#include "MantidKernel/BoundedValidator.h"

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
      return "Diagnostics;Workflow\\Diagnostics";
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
      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
                      "Name of the integrated detector vanadium (white beam) workspace." );
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","", Direction::Output),
                      "A MaskWorkspace containing the masked spectra as zeroes and ones.");
      auto mustBePosInt = boost::make_shared<BoundedValidator<int> >();
      mustBePosInt->setLower(0);
      declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
                      "The index number of the first spectrum to include in the calculation\n"
                      "(default 0)" );
      declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
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
      auto mustBePositiveDbl = boost::make_shared<BoundedValidator<double> >();
      mustBePositiveDbl->setLower(0);
      declareProperty("LevelsUp",0,mustBePosInt,"Levels above pixel that will be used to compute the median.\n"
                      "If no level is specified, or 0, the median is over the whole instrument.");
      this->setPropertyGroup("LevelsUp", medianDetTestGrp);
      declareProperty("SignificanceTest", 0.0, mustBePositiveDbl,
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

      string detEffVarGrp("Detector Efficiency Variation");
      declareProperty(new WorkspaceProperty<>("DetVanCompare","",Direction::Input, PropertyMode::Optional),
                      "Name of a matching second detector vanadium run from the same\n"
                      "instrument. It must be treated in the same manner as the input detector vanadium." );
      this->setPropertyGroup("DetVanCompare", detEffVarGrp);
      declareProperty("DetVanRatioVariation", 1.1, mustBePositiveDbl,
                      "Identify spectra whose total number of counts has changed by more\n"
                      "than this factor of the median change between the two input workspaces" );
      this->setPropertyGroup("DetVanRatioVariation", detEffVarGrp);
      setPropertySettings("DetVanRatioVariation", new EnabledWhenProperty("DetVanCompare", IS_NOT_DEFAULT));

      string countsCheck("Check Sample Counts");
      declareProperty(new WorkspaceProperty<>("SampleTotalCountsWorkspace", "",
          Direction::Input, PropertyMode::Optional),
          "A sample workspace integrated over the full axis range.");
      this->setPropertyGroup("SampleTotalCountsWorkspace", countsCheck);

      string backgroundCheck("Check Sample Background");
      declareProperty(new WorkspaceProperty<>("SampleBackgroundWorkspace", "",
          Direction::Input, PropertyMode::Optional),
          "A sample workspace integrated over the background region.");
      this->setPropertyGroup("SampleBackgroundWorkspace", backgroundCheck);
      declareProperty("SampleBkgLowAcceptanceFactor", 0.0, mustBePositiveDbl,
          "Low threshold for the background check MedianDetectorTest.");
      this->setPropertyGroup("SampleBkgLowAcceptanceFactor", backgroundCheck);
      declareProperty("SampleBkgHighAcceptanceFactor", 5.0, mustBePositiveDbl,
          "High threshold for the background check MedianDetectorTest.");
      this->setPropertyGroup("SampleBkgHighAcceptanceFactor", backgroundCheck);
      declareProperty("SampleBkgSignificanceTest", 3.3, mustBePositiveDbl,
                      "Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                      "difference with respect to the median value must also exceed this number of error bars");
      this->setPropertyGroup("SampleBkgSignificanceTest", backgroundCheck);

      string psdBleedMaskGrp("Create PSD Bleed Mask");
      declareProperty(new WorkspaceProperty<>("SampleWorkspace", "",
          Direction::Input, PropertyMode::Optional),
          "A sample workspace. This is used in the PSD Bleed calculation.");
      this->setPropertyGroup("SampleWorkspace", psdBleedMaskGrp);
      declareProperty("MaxTubeFramerate", 0.0, mustBePositiveDbl,
          "The maximum rate allowed for a tube in counts/us/frame.");
      this->setPropertyGroup("MaxTubeFramerate", psdBleedMaskGrp);
      declareProperty("NIgnoredCentralPixels", 80, mustBePosInt,
          "The number of pixels about the centre to ignore.");
      this->setPropertyGroup("NIgnoredCentralPixels", psdBleedMaskGrp);
      setPropertySettings("NIgnoredCentralPixels", new EnabledWhenProperty("MaxTubeFramerate", IS_NOT_DEFAULT));

      declareProperty("NumberOfFailures", 0, Direction::Output);
    }

    void DetectorDiagnostic::exec()
    {
      // get the generic information that everybody uses
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      m_minIndex = getProperty("StartWorkspaceIndex");
      m_maxIndex = getProperty("EndWorkspaceIndex");
      m_rangeLower = getProperty("RangeLower");
      m_rangeUpper = getProperty("RangeUpper");

      // integrate the data once to pass to subalgorithms
      m_fracDone = 0.;
      MatrixWorkspace_sptr countsWS = integrateSpectra(inputWS, m_minIndex, m_maxIndex,
                                                       m_rangeLower, m_rangeUpper, true);

      // calculate the number of tests for progress bar
      m_progStepWidth = 0;
      {
        int numTests(1); // if detector vanadium present, do it!
        if (!getPropertyValue("WhiteBeamCompare").empty())
          numTests += 1;
        if (!getPropertyValue("SampleTotalCountsWorkspace").empty())
          numTests += 1;
        if (!getPropertyValue("SampleBackgroundWorkspace").empty())
          numTests += 1;
        double temp = getProperty("MaxTubeFramerate");
        if (temp > 0. && !getPropertyValue("SampleWorkspace").empty())
          numTests += 1;

        m_progStepWidth = (1.-m_fracDone) / static_cast<double>(numTests);
      }

      int numFailed(0);
      MatrixWorkspace_sptr maskWS;

      // Perform FindDetectorsOutsideLimits and MedianDetectorTest on the
      // detector vanadium
      this->doDetVanTest(countsWS, maskWS, numFailed);

      // DetectorEfficiencyVariation (only if two workspaces are specified)
      if (!getPropertyValue("DetVanCompare").empty())
      {
          MatrixWorkspace_sptr input2WS = getProperty("DetVanCompare");

          MatrixWorkspace_sptr compareWS = integrateSpectra(input2WS, m_minIndex, m_maxIndex,
              m_rangeLower, m_rangeUpper, true);

          // apply mask to what we are going to input
          applyMask(compareWS, maskWS);

          this->doDetVanTest(compareWS, maskWS, numFailed);

          // get the relevant inputs
          double variation = getProperty("DetVanRatioVariation");

          // run the subalgorithm
          IAlgorithm_sptr alg = createSubAlgorithm("DetectorEfficiencyVariation", m_fracDone, m_fracDone+m_progStepWidth);
          m_fracDone += m_progStepWidth;
          alg->setProperty("WhiteBeamBase", countsWS);
          alg->setProperty("WhiteBeamCompare", compareWS);
          alg->setProperty("StartWorkspaceIndex", m_minIndex);
          alg->setProperty("EndWorkspaceIndex", m_maxIndex);
          alg->setProperty("RangeLower", m_rangeLower);
          alg->setProperty("RangeUpper", m_rangeUpper);
          alg->setProperty("Variation", variation);
          alg->executeAsSubAlg();
          MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
          maskWS += localMaskWS;
          int localFails = alg->getProperty("NumberOfFailures");
          numFailed += localFails;
      }

      // Zero total counts check for sample counts
      if (!getPropertyValue("SampleTotalCountsWorkspace").empty())
        {
          g_log.warning() << "DetDiag using total counts" << std::endl;
          MatrixWorkspace_sptr totalCountsWS = getProperty("SampleTotalCountsWorkspace");
          // apply mask to what we are going to input
          applyMask(totalCountsWS, maskWS);

          IAlgorithm_sptr zeroChk = createSubAlgorithm("FindDetectorsOutsideLimits", m_fracDone, m_fracDone+m_progStepWidth);
          m_fracDone += m_progStepWidth;
          zeroChk->setProperty("InputWorkspace", totalCountsWS);
          zeroChk->setProperty("StartWorkspaceIndex", m_minIndex);
          zeroChk->setProperty("EndWorkspaceIndex", m_maxIndex);
          zeroChk->setProperty("LowThreshold", 1.0e-10);
          zeroChk->setProperty("HighThreshold", 1.0e100);
          zeroChk->executeAsSubAlg();
          MatrixWorkspace_sptr localMaskWS = zeroChk->getProperty("OutputWorkspace");
          maskWS += localMaskWS;
          int localFails = zeroChk->getProperty("NumberOfFailures");
          numFailed += localFails;
          MaskWorkspace_sptr m = boost::dynamic_pointer_cast<MaskWorkspace>(localMaskWS);
          if (m)
            {
              g_log.information() << "A: " << m->getNumberMasked() << std::endl;
            }
        }

      // Background check
      if (!getPropertyValue("SampleBackgroundWorkspace").empty())
        {
          MatrixWorkspace_sptr bkgWS = getProperty("SampleBackgroundWorkspace");
          // apply mask to what we are going to input
          applyMask(bkgWS, maskWS);

          double significanceTest = getProperty("SampleBkgSignificanceTest");
          double lowThreshold = getProperty("SampleBkgLowAcceptanceFactor");
          double highThreshold = getProperty("SampleBkgHighAcceptanceFactor");

          // run the subalgorithm
          IAlgorithm_sptr alg = createSubAlgorithm("MedianDetectorTest", m_fracDone, m_fracDone+m_progStepWidth);
          m_fracDone += m_progStepWidth;
          alg->setProperty("InputWorkspace", bkgWS);
          alg->setProperty("StartWorkspaceIndex", m_minIndex);
          alg->setProperty("EndWorkspaceIndex", m_maxIndex);
          alg->setProperty("SignificanceTest", significanceTest);
          alg->setProperty("LowThreshold", lowThreshold);
          alg->setProperty("HighThreshold", highThreshold);
          alg->setProperty("LowOutlier", 0.0);
          alg->setProperty("HighOutlier", 1.0e100);
          alg->setProperty("ExcludeZeroesFromMedian", true);
          alg->executeAsSubAlg();
          MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
          maskWS += localMaskWS;
          int localFails = alg->getProperty("NumberOfFailures");
          numFailed += localFails;
        }

      // CreatePSDBleedMask (if selected)
      double maxTubeFrameRate = getProperty("MaxTubeFramerate");
      if (maxTubeFrameRate > 0. && !getPropertyValue("SampleWorkspace").empty())
      {
          MatrixWorkspace_sptr sampleWS = getProperty("SampleWorkspace");
          // get the relevant inputs
          int numIgnore = getProperty("NIgnoredCentralPixels");

          // run the subalgorithm
          IAlgorithm_sptr alg = createSubAlgorithm("CreatePSDBleedMask", m_fracDone, m_fracDone+m_progStepWidth);
          m_fracDone += m_progStepWidth;
          alg->setProperty("InputWorkspace", sampleWS);
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

      MaskWorkspace_sptr m = boost::dynamic_pointer_cast<MaskWorkspace>(maskWS);
      if (m)
        {
          g_log.information() << "B: " << m.get()->getNumberMasked() << std::endl;
        }
      setProperty("OutputWorkspace", maskWS);
    }

    void DetectorDiagnostic::applyMask(API::MatrixWorkspace_sptr inputWS,
                                       API::MatrixWorkspace_sptr maskWS)
    {
      IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors"); // should set progress bar
      maskAlg->setProperty("Workspace", inputWS);
      maskAlg->setProperty("MaskedWorkspace", maskWS);
      maskAlg->setProperty("StartWorkspaceIndex", m_minIndex);
      maskAlg->setProperty("EndWorkspaceIndex", m_maxIndex);
      maskAlg->executeAsSubAlg();
    }

    void DetectorDiagnostic::doDetVanTest(API::MatrixWorkspace_sptr inputWS,
        API::MatrixWorkspace_sptr &maskWS, int &nFails)
    {
      // FindDetectorsOutsideLimits
      // get the relevant inputs
      double lowThreshold = getProperty("LowThreshold");
      double highThreshold = getProperty("HighThreshold");

      // run the subalgorithm
      IAlgorithm_sptr fdol = createSubAlgorithm("FindDetectorsOutsideLimits", m_fracDone, m_fracDone+m_progStepWidth);
      m_fracDone += m_progStepWidth;
      fdol->setProperty("InputWorkspace", inputWS);
      fdol->setProperty("StartWorkspaceIndex", m_minIndex);
      fdol->setProperty("EndWorkspaceIndex", m_maxIndex);
      fdol->setProperty("RangeLower", m_rangeLower);
      fdol->setProperty("RangeUpper", m_rangeUpper);
      fdol->setProperty("LowThreshold", lowThreshold);
      fdol->setProperty("HighThreshold", highThreshold);
      fdol->executeAsSubAlg();
      maskWS = fdol->getProperty("OutputWorkspace");
      MaskWorkspace_sptr m = boost::dynamic_pointer_cast<MaskWorkspace>(maskWS);
      if (m)
        {
          g_log.information() << "A: " << m.get()->getNumberMasked() << std::endl;
        }
      int localFails = fdol->getProperty("NumberOfFailures");
      nFails += localFails;

      // get the relevant inputs for the MedianDetectorTests
      int parents = getProperty("LevelsUp");
      double significanceTest = getProperty("SignificanceTest");
      double lowThresholdFrac = getProperty("LowThresholdFraction");
      double highThresholdFrac = getProperty("HighThresholdFraction");
      double lowOutlier = getProperty("LowOutlier");
      double highOutlier = getProperty("HighOutlier");
      bool excludeZeroes = getProperty("ExcludeZeroesFromMedian");

      // MedianDetectorTest
      // apply mask to what we are going to input
      applyMask(inputWS, maskWS);

      // run the subalgorithm
      IAlgorithm_sptr mdt = createSubAlgorithm("MedianDetectorTest", m_fracDone, m_fracDone+m_progStepWidth);
      m_fracDone += m_progStepWidth;
      mdt->setProperty("InputWorkspace", inputWS);
      mdt->setProperty("StartWorkspaceIndex", m_minIndex);
      mdt->setProperty("EndWorkspaceIndex", m_maxIndex);
      mdt->setProperty("RangeLower", m_rangeLower);
      mdt->setProperty("RangeUpper", m_rangeUpper);
      mdt->setProperty("LevelsUp", parents);
      mdt->setProperty("SignificanceTest", significanceTest);
      mdt->setProperty("LowThreshold", lowThresholdFrac);
      mdt->setProperty("HighThreshold", highThresholdFrac);
      mdt->setProperty("LowOutlier", lowOutlier);
      mdt->setProperty("HighOutlier", highOutlier);
      mdt->setProperty("ExcludeZeroesFromMedian", excludeZeroes);
      mdt->executeAsSubAlg();
      MatrixWorkspace_sptr localMaskWS = mdt->getProperty("OutputWorkspace");
      m = boost::dynamic_pointer_cast<MaskWorkspace>(localMaskWS);
      if (m)
        {
          g_log.information() << "A: " << m.get()->getNumberMasked() << std::endl;
        }

      maskWS += localMaskWS;
      localFails = mdt->getProperty("NumberOfFailures");
      nFails += localFails;
    }

    //--------------------------------------------------------------------------
    // Public member functions
    //--------------------------------------------------------------------------
    DetectorDiagnostic::DetectorDiagnostic() : 
      API::Algorithm(), m_fracDone(0.0), m_TotalTime(RTTotal), m_parents(0),
      m_progStepWidth(0.0), m_minIndex(0), m_maxIndex(EMPTY_INT()),
      m_rangeLower(EMPTY_DBL()), m_rangeUpper(EMPTY_DBL())
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
     */
    DataObjects::MaskWorkspace_sptr DetectorDiagnostic::generateEmptyMask(API::MatrixWorkspace_const_sptr inputWS)
    {
      // Create a new workspace for the results, copy from the input to ensure that we copy over the instrument and current masking
      DataObjects::MaskWorkspace_sptr maskWS(new DataObjects::MaskWorkspace());
      maskWS->initialize(inputWS->getNumberHistograms(), 1, 1);
      WorkspaceFactory::Instance().initializeFromParent(inputWS, maskWS, false);
      maskWS->setTitle(inputWS->getTitle());

      return maskWS;
    }


    std::vector<std::vector<size_t> > DetectorDiagnostic::makeInstrumentMap(API::MatrixWorkspace_sptr countsWS)
    {
      std::vector<std::vector<size_t> > mymap;
      std::vector<size_t> single;

      for(size_t i=0;i < countsWS->getNumberHistograms();i++)
      {
        single.push_back(i);
      }
      mymap.push_back(single);
      return mymap;
    }
    /** This function will check how to group spectra when calculating median
      *
      *
      */
    std::vector<std::vector<size_t> > DetectorDiagnostic::makeMap(API::MatrixWorkspace_sptr countsWS)
    {
      std::multimap<Mantid::Geometry::ComponentID,size_t> mymap;

      Geometry::Instrument_const_sptr instrument = countsWS->getInstrument();
      if (m_parents==0)
      {
        return makeInstrumentMap(countsWS);
      }
      if (!instrument)
      {
        g_log.warning("Workspace has no instrument. LevelsUP is ignored");
        return makeInstrumentMap(countsWS);
      }

      //check if not grouped. If grouped, it will throw
      try
      {
        detid2index_map *d2i=countsWS->getDetectorIDToWorkspaceIndexMap(true);
        d2i->clear();
      }
      catch(...)
      {
        throw std::runtime_error("Median detector test: not able to create detector to spectra map. Try with LevelUp=0.");
      }


      for(size_t i=0;i < countsWS->getNumberHistograms();i++)
      {
        detid_t d=(*((countsWS->getSpectrum(i))->getDetectorIDs().begin()));
        std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent> > anc=instrument->getDetector(d)->getAncestors();
        //std::vector<boost::shared_ptr<const IComponent> > anc=(*(countsWS->getSpectrum(i)->getDetectorIDs().begin()))->getAncestors();
        if (anc.size()<static_cast<size_t>(m_parents))
        {
          g_log.warning("Too many levels up. Will ignore LevelsUp");
          m_parents=0;
          return makeInstrumentMap(countsWS);
        }
        mymap.insert(std::pair<Mantid::Geometry::ComponentID,size_t>(anc[m_parents-1]->getComponentID(),i));
      }

      std::vector<std::vector<size_t> > speclist;
      std::vector<size_t>  speclistsingle;

      std::multimap<Mantid::Geometry::ComponentID,size_t>::iterator m_it, s_it;

      for (m_it = mymap.begin();  m_it != mymap.end();  m_it = s_it)
      {
        Mantid::Geometry::ComponentID theKey = (*m_it).first;

        std::pair<std::multimap<Mantid::Geometry::ComponentID,size_t>::iterator,std::multimap<Mantid::Geometry::ComponentID,size_t>::iterator> keyRange = mymap.equal_range(theKey);

        // Iterate over all map elements with key == theKey
        speclistsingle.clear();
        for (s_it = keyRange.first;  s_it != keyRange.second;  ++s_it)
        {
          speclistsingle.push_back( (*s_it).second );
        }
        speclist.push_back(speclistsingle);
      }

      return speclist;
    }
    /** 
     *  Finds the median of values in single bin histograms rejecting spectra from masked
     *  detectors and the results of divide by zero (infinite and NaN).  
     * The median is an average that is less affected by small numbers of very large values.
     * @param input :: A histogram workspace with one entry in each bin
     * @param excludeZeroes :: If true then zeroes will not be included in the median calculation
     * @return The median value of the histograms in the workspace that was passed to it
     * @throw out_of_range if a value is negative
     */
    std::vector<double> DetectorDiagnostic::calculateMedian(const API::MatrixWorkspace_sptr input, bool excludeZeroes, std::vector<std::vector<size_t> > indexmap)
    {
      std::vector<double> medianvec;
      g_log.debug("Calculating the median count rate of the spectra");

      for (size_t j=0;  j< indexmap.size(); ++j)
      {
        std::vector<double> medianInput;
        std::vector<size_t> hists=indexmap.at(j);

        const int nhists = static_cast<int>(hists.size());
        // The maximum possible length is that of workspace length
        medianInput.reserve(nhists);

        bool checkForMask = false;
        Geometry::Instrument_const_sptr instrument = input->getInstrument();
        if (instrument != NULL)
        {
          checkForMask = ((instrument->getSource() != NULL) && (instrument->getSample() != NULL));
        }

        PARALLEL_FOR1(input)
        for (int i = 0; i<static_cast<int>(hists.size()); ++i)
        {
          PARALLEL_START_INTERUPT_REGION

          if (checkForMask) {
            const std::set<detid_t>& detids = input->getSpectrum(hists[i])->getDetectorIDs();
            if (instrument->isDetectorMasked(detids))
              continue;
            if (instrument->isMonitor(detids))
              continue;
          }

          const double yValue = input->readY(hists[i])[0];
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
            g_log.information("some group has no valid histograms. Will use 0 for median.");
            medianInput.push_back(0.);
        }

        // We need a sorted array to calculate the median
        std::sort(medianInput.begin(), medianInput.end());
        double median = gsl_stats_median_from_sorted_data( &medianInput[0], 1, medianInput.size() );

        if ( median < 0 || median > DBL_MAX/10.0 )
        {
          throw std::out_of_range("The calculated value for the median was either negative or unreliably large");
        }
        medianvec.push_back(median);
      }
      return medianvec;
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
