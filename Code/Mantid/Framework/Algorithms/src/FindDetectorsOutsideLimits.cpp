#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include <fstream>
#include <cmath>

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(FindDetectorsOutsideLimits)
      
    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    /// Initialisation method.
    void FindDetectorsOutsideLimits::init()
    {
      this->setOptionalMessage(
          "Identifies histograms and their detectors that have total numbers\n"
          "of counts over a user defined maximum or less than the user define minimum." );

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
          "Name of the input workspace2D" );
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
          "Each histogram from the input workspace maps to a histogram in this\n"
          "workspace with one value that indicates if there was a dead detector" );
      declareProperty("HighThreshold", 0.0,
          "Spectra whose total number of counts are equal to or above this value\n"
          "will be marked bad (default 0)" );
      declareProperty("LowThreshold", 0.0,
          "Spectra whose total number of counts are equal to or below this value\n"
          "will be marked bad (default 0)" );
      declareProperty("RangeLower", EMPTY_DBL(),
          "No bin with a boundary at an x value less than this will be used\n"
          "in the summation that decides if a detector is 'bad' (default: the\n"
          "start of each histogram)" );
      declareProperty("RangeUpper", EMPTY_DBL(),
          "No bin with a boundary at an x value higher than this value will\n"
          "be used in the summation that decides if a detector is 'bad'\n"
          "(default: the end of each histogram)" );
      declareProperty("NumberOfFailures", 0, Direction::Output);
    }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if the algorithm cannot execute
     *  @throw invalid_argument is the LowThreshold property is greater than HighThreshold
     */
    void FindDetectorsOutsideLimits::exec()
    {
      double lowThreshold = getProperty("LowThreshold");
      double highThreshold = getProperty("HighThreshold");
      if ( highThreshold <= lowThreshold )
      {
        g_log.error() << name() << ": Lower limit (" << lowThreshold <<
            ") >= the higher limit (" << highThreshold <<
            "), all detectors in the spectrum would be marked bad";
        throw std::invalid_argument("The high threshold must be higher than the low threshold");
      }

      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      const double rangeLower = getProperty("RangeLower");
      const double rangeUpper = getProperty("RangeUpper");

      // Get the integrated input workspace; converting to a Workspace2D
      MatrixWorkspace_sptr integratedWorkspace = 
          integrateSpectra(inputWS, 0, EMPTY_INT(), rangeLower, rangeUpper, true);

      EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>( integratedWorkspace);
      if (ew)
        throw std::runtime_error("Error! Integration output is not a Workspace2D.");

      //set the workspace to have no units
      integratedWorkspace->isDistribution(false);
      integratedWorkspace->setYUnit("");


      const double liveValue(1.0);
      // iterate over the data values setting the live and dead values
      const int numSpec = integratedWorkspace->getNumberHistograms();
      const int progStep = static_cast<int>(ceil(numSpec / 100.0));

      // Keep track of those reading low/high
      size_t numLow(0), numHigh(0);

      for (int i = 0; i < numSpec; ++i)
      {
        if (i % progStep == 0)
        {
          progress(static_cast<double>(i)/numSpec);
          interruption_point();
        }

        const double & yValue = integratedWorkspace->readY(i)[0];
        if ( yValue <= lowThreshold )
        {
          integratedWorkspace->maskWorkspaceIndex(i);
          ++numLow;
          continue;
        }

        if ( yValue >= highThreshold )
        {
          integratedWorkspace->maskWorkspaceIndex(i);
          ++numHigh;
          continue;
        }

        // Value passed the tests, flag it.
        integratedWorkspace->dataY(i)[0] = liveValue;
      }
	
      const int totalNumFailed(numLow+numHigh);
      g_log.information() << "Found a total of " << totalNumFailed << " failing "
			  << "spectra (" << numLow << " reading low and " << numHigh
			  << " reading high)" << std::endl;
  
      setProperty("NumberOfFailures", totalNumFailed);
      // Assign it to the output workspace property
      setProperty("OutputWorkspace", integratedWorkspace);
    }

  }
}
