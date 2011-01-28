//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/DetectorDiagnostic.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Exception.h"
#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <gsl/gsl_statistics.h>
#include <cfloat>

namespace Mantid
{

  namespace Algorithms
  {

    using API::MatrixWorkspace_sptr;
    using API::IAlgorithm_sptr;
    using Geometry::IDetector_sptr;
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
					 const int indexMin,
					 const int indexMax,
					 const double lower, 
					 const double upper,
					 const bool outputWorkspace2D)
    {
      g_log.information() << "Integrating input spectra.\n";
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
      try

      {
        // Now execute integrate
        childAlg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Exception thrown while running the Integration sub-algorithm");
        throw;
      }

      if ( ! childAlg->isExecuted() )
      {
        g_log.error("The Integration algorithm failed unexpectedly, aborting.");
        throw std::runtime_error(name() + " failed trying to run Integration");
      }

      // Convert to 2D if desired, and if the input was an EventWorkspace.
      MatrixWorkspace_sptr outputW = childAlg->getProperty("OutputWorkspace");
      if (outputWorkspace2D && boost::dynamic_pointer_cast<EventWorkspace>(outputW))
      {
        g_log.debug() << "Converting output Event Workspace into a Workspace2D." << std::endl;
        outputW = EventWorkspaceHelpers::convertEventTo2D(outputW);
      }

      return outputW;
    }

    
    /** 
     *  Fnds the median of values in single bin histograms rejecting spectra from masked
     *  detectors and the results of divide by zero (infinite and NaN).  
     * The median is an average that is less affected by small numbers of very large values.
     * @param input :: A histogram workspace with one entry in each bin
     * @param badIndices :: [Output] A set filled with indices to skip when performing tests
     * @return The median value of the histograms in the workspace that was passed to it
     * @throw out_of_range if a value is incountered that is unbelievibly high or negative
     */
    double DetectorDiagnostic::calculateMedian(const API::MatrixWorkspace_sptr input, 
					       std::set<int> & badIndices)
    {
      g_log.information("Calculating the median count rate of the spectra");

      // The median should only include "good" spectra, i.e. those not INF, NAN or without
      // a detector
      std::vector<double> goodValues;
      const int nhists(input->getNumberHistograms());
      // The maximum possible length is that of workspace length
      goodValues.reserve(nhists);
      // Track the indices or the bad detectors so that we don't need to double check later
      badIndices.clear();      

      PARALLEL_FOR1(input)
      for (int i = 0; i < nhists; ++i)
      {
	PARALLEL_START_INTERUPT_REGION
	  
	IDetector_sptr det;
	try
	{
	  det = input->getDetector(i);
	}
	catch (Kernel::Exception::NotFoundError&)
	{
	  PARALLEL_CRITICAL(DetectorDiagnostic_median_a)
	  {
	    badIndices.insert(i);
	  }
	  continue;
	}
	// If we're already masked skip it
	if( det->isMasked() )
	{
	  PARALLEL_CRITICAL(DetectorDiagnostic_median_b)
	  {
	    badIndices.insert(i);
	  }
	  continue;
	}

	const double yValue = input->readY(i)[0];
	// We shouldn't have negative numbers of counts, probably a SolidAngle correction problem
	if ( yValue  < 0 )
	{
	  g_log.debug() << "Negative count rate found for spectrum index " << i << std::endl;
	  throw std::out_of_range("Negative number of counts found, could be corrupted raw counts or solid angle data");
	}
	// There has been a divide by zero, likely to be due to a detector with zero solid angle
	if( boost::math::isinf(yValue) || boost::math::isnan(yValue) )
	{
	  PARALLEL_CRITICAL(DetectorDiagnostic_median_c)
	  {
	    badIndices.insert(i);
	  }
	  continue;
	}
	// Now we have a good value
	PARALLEL_CRITICAL(DetectorDiagnostic_median_d)
	{
	  goodValues.push_back(yValue);
	}
	
	PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // We need a sorted array to calculate the median
      std::sort(goodValues.begin(), goodValues.end());
      double median = gsl_stats_median_from_sorted_data( &goodValues[0], 1, goodValues.size() );
  
      if ( median < 0 || median > DBL_MAX/10.0 )
      {
	throw std::out_of_range("The calculated value for the median was either negative or unreliably large");
      }
      return median;
    }

    /** 
     * Convert to a distribution
     * @param workspace :: The input workspace to convert to a count rate
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
