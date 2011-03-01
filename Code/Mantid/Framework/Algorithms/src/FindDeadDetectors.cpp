//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <fstream>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(FindDeadDetectors)

    using namespace Kernel;
    using namespace API;

    /// Initialisation method.
    void FindDeadDetectors::init()
    {
      //this->setWikiSummary("Identifies and flags empty spectra caused by 'dead' detectors.");
      //this->setOptionalMessage("Identifies and flags empty spectra caused by 'dead' detectors.");

      declareProperty(
        new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
        "Name of the input workspace" );
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "Each histogram from the input workspace maps to a histogram in this\n"
        "workspace with one value that indicates if there was a dead detector" );

      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0);
      declareProperty("DeadThreshold",0.0, mustBePositive,
        "The threshold against which to judge if a spectrum belongs to a dead\n"
        "detector" );
      // As the property takes ownership of the validator pointer, have to take care to pass in a unique
      // pointer to each property.
      declareProperty("LiveValue",0.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'live'\n"
        "(default 0.0)");
      declareProperty("DeadValue",100.0, mustBePositive->clone(),
        "The value to assign to an integrated spectrum flagged as 'dead'\n"
        "(default 100.0)" );
      //EMPTY_DBL() is a tag that tells us that no value has been set and we want to use the default
      declareProperty("RangeLower", EMPTY_DBL(),
        "No bin with a boundary at an x value less than this will be used\n"
        "in the summation that decides if a detector is 'dead' (default: the\n"
        "start of each histogram)" );
      declareProperty("RangeUpper", EMPTY_DBL(),
        "No bin with a boundary at an x value higher than this value will\n"
        "be used in the summation that decides if a detector is 'dead'\n"
        "(default: the end of each histogram)" );
      declareProperty("OutputFile","",
        "A filename to which to write the list of dead detector UDETs" );
      // This output property will contain the list of UDETs for the dead detectors
      declareProperty("FoundDead",std::vector<int>(),Direction::Output);
    }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
    void FindDeadDetectors::exec()
    {
      double deadThreshold = getProperty("DeadThreshold");
      double liveValue = getProperty("LiveValue");
      double deadValue = getProperty("DeadValue");

      // Try and open the output file, if specified, and write a header
      std::ofstream file(getPropertyValue("OutputFile").c_str());
      file << "Index Spectrum UDET(S)" << std::endl;

      // Get the integrated input workspace
      MatrixWorkspace_sptr integratedWorkspace = integrateWorkspace(getPropertyValue("OutputWorkspace"));

      // Get hold of the spectraDetectorMap and axis
      const SpectraDetectorMap& specMap = integratedWorkspace->spectraMap();
      Axis* specAxis = integratedWorkspace->getAxis(1);

      std::vector<int> deadDets;
      int countSpec = 0, countDets = 0;


      // iterate over the data values setting the live and dead values
      g_log.information() << "Marking dead detectors" << std::endl;
      const int numSpec = integratedWorkspace->getNumberHistograms();
      int iprogress_step = numSpec / 100;
      if (iprogress_step == 0) iprogress_step = 1;
      for (int i = 0; i < numSpec; ++i)
      {
        double &y = integratedWorkspace->dataY(i)[0];
        if ( y > deadThreshold )
        {
        y = liveValue;
        }
        else
        {
          ++countSpec;
          y = deadValue;
          const int specNo = specAxis->spectraNo(i);
          // Write the spectrum number to file
          file << i << " " << specNo;
          // Get the list of detectors for this spectrum and iterate over
          const std::vector<int> dets = specMap.getDetectors(specNo);
          std::vector<int>::const_iterator it;
          for (it = dets.begin(); it != dets.end(); ++it)
          {
            // Write the detector ID to file, log & the FoundDead output property
            file << " " << *it;
            //we could write dead detectors to the log but if they are viewing the log in the MantidPlot viewer it will crash MantidPlot
            deadDets.push_back(*it);
            ++countDets;
          }
          file << std::endl;
        }
        if (i % iprogress_step == 0)
        {
            progress(double(i)/numSpec);
            interruption_point();
        }
      }

      g_log.information() << "Found a total of " << countDets << " 'dead' detectors within "
                          << countSpec << " 'dead' spectra." << std::endl;

      // Assign it to the output workspace property
      setProperty("OutputWorkspace",integratedWorkspace);
      setProperty("FoundDead",deadDets);

      // Close the output file
      file.close();
      return;
    }

    /// Run Integration as a sub-algorithm
    MatrixWorkspace_sptr FindDeadDetectors::integrateWorkspace(std::string outputWorkspaceName)
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

  } // namespace Algorithm
} // namespace Mantid
