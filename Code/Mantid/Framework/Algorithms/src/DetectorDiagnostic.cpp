//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/DetectorDiagnostic.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Exception.h"
#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <gsl/gsl_statistics.h>
#include <cfloat>

namespace Mantid
{

  namespace Algorithms
  {

    using API::MatrixWorkspace_sptr;
    using API::IAlgorithm_sptr;
    using Geometry::IDetector_const_sptr;
    using namespace Mantid::DataObjects;
    using namespace Mantid::API;

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
      std::cout << "checkForMask = " << checkForMask << std::endl; // REMOVE

      PARALLEL_FOR1(input)
      for (int i = 0; i < nhists; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        if (checkForMask) {
          if (instrument->isDetectorMasked(i))
            continue;
          if (instrument->isMonitor(i))
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
