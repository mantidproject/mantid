/*WIKI* 


This is intended to identify detectors that are grossly over or under counting. It reads the input workspace and identifies all histograms with numbers of counts outside the user defined upper and lower limits.  Each spectra that fails has its spectra masked on the output workspace. Spectra that pass the test have their data set to a positive value, 1.0. The output workspace can be fed to [[MaskDetectors]] to mask the same spectra on another workspace.


====Subalgorithms used====

Uses the [[Integration]] algorithm to sum the spectra.


*WIKI*/
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
    
    /// Sets documentation strings for this algorithm
    void FindDetectorsOutsideLimits::initDocs()
    {
      this->setWikiSummary("Identifies histograms and their detectors that have total numbers of counts over a user defined maximum or less than the user define minimum. ");
      this->setOptionalMessage("Identifies histograms and their detectors that have total numbers of counts over a user defined maximum or less than the user define minimum.");
    }
    
      
    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    /// Initialisation method.
    void FindDetectorsOutsideLimits::init()
    {
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
          "Name of the input workspace2D" );
      declareProperty(
          new WorkspaceProperty<DataObjects::SpecialWorkspace2D>("OutputWorkspace","",Direction::Output),
          "Each histogram from the input workspace maps to a histogram in this\n"
          "SpecialWorkspace2D (Masking workdpace) with one value that indicates if there was a dead detector" );
      /*
       declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace2", "", Direction::Output),
          "Same as OutputWorksapce, but in Workspace2D");
      */
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
      // 1. Get properties (inputs)
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

      // 2. Create output workspace and set it up & check input workspace size
      Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
      DataObjects::SpecialWorkspace2D_sptr maskWS(new DataObjects::SpecialWorkspace2D(instrument));
      this->setProperty("OutputWorkspace", maskWS);

      if (maskWS->getNumberHistograms() > inputWS->getNumberHistograms()){
        g_log.warning() << "Input Workspace " << inputWS->getName() << " has different number of spectra (" <<
            inputWS->getNumberHistograms() << ") to new generated masking workspace (" <<
            maskWS->getNumberHistograms() << ").  " << "  Input workspace might be focused" << std::endl;
      }
      else if (maskWS->getNumberHistograms() < inputWS->getNumberHistograms()){
        g_log.error() << "Input Workspace " << inputWS->getName() << " has different number of spectra (" <<
            inputWS->getNumberHistograms() << ") to new generated masking workspace (" <<
            maskWS->getNumberHistograms() << ").  " << "  Input workspace might be focused" << std::endl;
        throw std::runtime_error("Error! This cannot be right.  Instrument must be wrong.");
      }

      // 3. Get the integrated input workspace; converting to a Workspace2D
      MatrixWorkspace_sptr integratedWorkspace = 
          integrateSpectra(inputWS, 0, EMPTY_INT(), rangeLower, rangeUpper, true);

      EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>(integratedWorkspace);
      if (ew)
        throw std::runtime_error("Error! Integration output is not a Workspace2D.");

      // i. set the workspace to have no units
      //    TODO remove the setup on integratedWorkspace after testing
      // integratedWorkspace->isDistribution(false);
      // integratedWorkspace->setYUnit("");

      // 4. Read integrated and set up the Masking workspace
      const double liveValue(1.0);
      const double deadValue = 0.0;

      // a) iterate over the data values setting the live and dead values
      const int numSpec = static_cast<int>(integratedWorkspace->getNumberHistograms());
      const int progStep = static_cast<int>(ceil(numSpec / 100.0));

      // b) Keep track of those reading low/high
      int numLow(0), numHigh(0);

      // c) Loop
      for (int i = 0; i < numSpec; ++i)
      {
        // i) Progress
        if (i % progStep == 0)
        {
          progress(static_cast<double>(i)/numSpec);
          interruption_point();
        }

        // ii) Set up value
        const double & yValue = integratedWorkspace->readY(i)[0];
        if ( yValue <= lowThreshold )
        {
          // Value failed
          integratedWorkspace->maskWorkspaceIndex(i);

          maskWS->dataY(i)[0] = deadValue;

          ++numLow;

        } else if ( yValue >= highThreshold )
        {
          // Value failed
          integratedWorkspace->maskWorkspaceIndex(i);

          maskWS->dataY(i)[0] = deadValue;

          ++numHigh;

        } else {
          // Value passed the tests, flag it.
          integratedWorkspace->dataY(i)[0] = liveValue;

          maskWS->dataY(i)[0] = liveValue;
        } // ENDIFELSE
      } // ENDFOR
        
      const int totalNumFailed(numLow+numHigh);
      g_log.notice() << "Found a total of " << totalNumFailed << " failing "
                     << "spectra (" << numLow << " reading low and " << numHigh
                     << " reading high)" << std::endl;
  
      setProperty("NumberOfFailures", totalNumFailed);
      // Assign it to the output workspace property
      // setProperty("OutputWorkspace2", integratedWorkspace);
      setProperty("OutputWorkspace", maskWS);

      return;
    }

  }
}
