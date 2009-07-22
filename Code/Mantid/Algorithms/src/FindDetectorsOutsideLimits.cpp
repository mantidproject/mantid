#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <fstream>

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(FindDetectorsOutsideLimits)

  using namespace Kernel;
  using namespace API;
  using DataObjects::Workspace2D;

  /// Initialisation method.
  void FindDetectorsOutsideLimits::init()
  {
    declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
      "Name of the input workspace2D" );
    declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
      "Each histogram from the input workspace maps to a histogram in this\n"
      "workspace with one value that indicates if there was a dead detector" );
    declareProperty("HighThreshold", EMPTY_DBL(),
        new MandatoryValidator<double>(),
        "Spectra whose total number of counts are above or equal to this value will be\n"
        "marked bad" );

    BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
    mustBePositive->setLower(0);
    declareProperty("LowThreshold",0.0, mustBePositive,
        "Spectra whose total number of counts are below or equal to this value will be\n"
        "marked bad (default 0)" );
      declareProperty("GoodValue",0.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'live'\n"
        "(default 0.0)");
      declareProperty("BadValue",100.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'bad'\n"
        "(default 100.0)" );
      declareProperty("RangeLower", EMPTY_DBL(),
        "No bin with a boundary at an x value less than this will be used\n"
        "in the summation that decides if a detector is 'bad' (default: the\n"
        "start of each histogram)" );
      declareProperty("RangeUpper", EMPTY_DBL(),
        "No bin with a boundary at an x value higher than this value will\n"
        "be used in the summation that decides if a detector is 'bad'\n"
        "(default: the end of each histogram)" );
      declareProperty("OutputFile","",
        "A filename to which to write the list of dead detector UDETs" );
      // This output property will contain the list of UDETs for the dead detectors
      declareProperty("FoundDead",std::vector<int>(),Direction::Output);
    }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if the algorithm cannot execute
     */
    void FindDetectorsOutsideLimits::exec()
    {
      double lowThreshold = getProperty("LowThreshold");
      double highThreshold = getProperty("HighThreshold");
      double liveValue = getProperty("GoodValue");
      double deadValue = getProperty("BadValue");

      // Try and open the output file, if specified, and write a header
      std::ofstream file(getPropertyValue("OutputFile").c_str());
      file << "Index Spectrum UDET(S)" << std::endl;

      // Get the integrated input workspace
      MatrixWorkspace_sptr integratedWorkspace = integrateWorkspace(getPropertyValue("OutputWorkspace"));

      // Get hold of the spectraDetectorMap and axis
      const SpectraDetectorMap& specMap = integratedWorkspace->spectraMap();
      Axis* specAxis = integratedWorkspace->getAxis(1);

      std::vector<int> deadDets;
      // get ready to report the number of bad detectors found to the log
      int countSpec = 0, cLows = 0, cHighs = 0;

      // iterate over the data values setting the live and dead values
      g_log.information() << "Marking dead detectors" << std::endl;
      const int numSpec = integratedWorkspace->getNumberHistograms();
      int iprogress_step = numSpec / 100;
      if (iprogress_step == 0) iprogress_step = 1;
      for (int i = 0; i < numSpec; ++i)
      {
        double &yInputOutput = integratedWorkspace->dataY(i)[0];
        // hold information about whether it passes or fails and why
        std::string problem = "";
        if ( yInputOutput <= lowThreshold )
        {
          problem = "low";
          cLows++;
        }
        if ( yInputOutput >= highThreshold )
        {
          problem = "high";
          cHighs++;
        }
        if ( problem == "" )
        {// it is an acceptable value; just write the good flag to the output workspace and go on to check the next value
          yInputOutput = liveValue;
        }
        else
        {
          ++countSpec;
          yInputOutput = deadValue;
          const int specNo = specAxis->spectraNo(i);
          // Write the spectrum number to file
          file << "Spectra number " << specNo << " is " << problem << ", detectors: ";
          // Get the list of detectors for this spectrum and iterate over
          const std::vector<int> dets = specMap.getDetectors(specNo);
          std::vector<int>::const_iterator it;
          for (it = dets.begin(); it != dets.end(); ++it)
          {
            // Write the detector ID to file
            file << " " << *it;
            //we could write dead detectors to the log but if they are viewing the log in the MantidPlot viewer it will crash MantidPlot
            deadDets.push_back(*it);
          }
          file << std::endl;
        }
        if (i % iprogress_step == 0)
        {
            progress(double(i)/numSpec);
            interruption_point();
        }
      }

      g_log.information() << "Found a total of " << cLows+cHighs << " 'dead' "
        << "detectors (" << cLows << " reading low and " << cHighs
        << " reading high) within " << countSpec << " 'dead' spectra"
        << std::endl;

      // Assign it to the output workspace property
      setProperty("OutputWorkspace",integratedWorkspace);
      setProperty("FoundDead",deadDets);

      // Close the output file
      file.close();
      return;
    }

    /// Run Integration as a sub-algorithm
    MatrixWorkspace_sptr FindDetectorsOutsideLimits::integrateWorkspace(std::string outputWorkspaceName)
    {
      g_log.information() << "Integrating input workspace" << std::endl;

      API::IAlgorithm_sptr childAlg = createSubAlgorithm("Integration");
      // Now execute integration. Catch and log any error
      try
      {
        //pass inputed values straight to Integration, checking must be done there
        childAlg->setPropertyValue( "InputWorkspace", getPropertyValue("InputWorkspace") );
        childAlg->setPropertyValue( "OutputWorkspace", outputWorkspaceName);
        childAlg->setPropertyValue( "RangeLower",  getPropertyValue("RangeLower") );
        childAlg->setPropertyValue( "RangeUpper", getPropertyValue("RangeUpper") );
        childAlg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Integration sub-algorithm");
        throw;
      }

      if ( ! childAlg->isExecuted() ) g_log.error("Unable to successfully run Integration sub-algorithm");

      MatrixWorkspace_sptr retVal = childAlg->getProperty("OutputWorkspace");

      return retVal;
    }

}
}
