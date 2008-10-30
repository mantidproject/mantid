//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidDataObjects/Workspace2D.h"
#include <fstream>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(FindDeadDetectors)

    using namespace Kernel;
    using namespace API;
    using DataObjects::Workspace2D;
//    using DataObjects::Workspace2D_sptr;

    // Get a reference to the logger
    Logger& FindDeadDetectors::g_log = Logger::get("FindDeadDetectors");

    /// Initialisation method.
    void FindDeadDetectors::init()
    {
      declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0);
      declareProperty("DeadThreshold",0.0, mustBePositive);
      // As the property takes ownership of the validator pointer, have to take care to pass in a unique
      // pointer to each property.
      declareProperty("LiveValue",0.0, mustBePositive->clone());
      declareProperty("DeadValue",100.0, mustBePositive->clone());

      declareProperty("OutputFile","");
      // This output property will contain the list of UDETs for the dead detectors
      declareProperty("FoundDead",std::vector<int>(),Direction::Output);
    }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
    void FindDeadDetectors::exec()
    {
      // Try and retrieve the optional properties
      double deadThreshold = getProperty("DeadThreshold");
      double liveValue = getProperty("LiveValue");
      double deadValue = getProperty("DeadValue");

      // Try and open the output file, if specified, and write a header
      std::ofstream file(getPropertyValue("OutputFile").c_str());
      file << "Index Spectrum UDET(S)" << std::endl;

      // Get the integrated input workspace
      Workspace_sptr integratedWorkspace = integrateWorkspace(getPropertyValue("OutputWorkspace"));

      // Get
      boost::shared_ptr<SpectraDetectorMap> specMap = integratedWorkspace->getSpectraMap();
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
          const std::vector<Geometry::IDetector*> dets = specMap->getDetectors(specNo);
          std::vector<Geometry::IDetector*>::const_iterator it;
          for (it = dets.begin(); it != dets.end(); ++it)
          {
            const int detID = (*it)->getID();
            // Write the detector ID to file, log & the FoundDead output property
            file << " " << detID;
            g_log.debug() << "Dead detector: " << detID << std::endl;
            deadDets.push_back(detID);
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
    Workspace_sptr FindDeadDetectors::integrateWorkspace(std::string outputWorkspaceName)
    {
      g_log.information() << "Integrating input workspace" << std::endl;

      API::Algorithm_sptr childAlg = createSubAlgorithm("Integration");
      childAlg->setPropertyValue("InputWorkspace", getPropertyValue("InputWorkspace"));
      childAlg->setPropertyValue("OutputWorkspace", outputWorkspaceName);

      // Now execute the sub-algorithm. Catch and log any error
      try
      {
        childAlg->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.error("Unable to successfully run Integration sub-algorithm");
        throw;
      }

      if ( ! childAlg->isExecuted() ) g_log.error("Unable to successfully run Integration sub-algorithm");

      Workspace_sptr retVal = childAlg->getProperty("OutputWorkspace");

      return retVal;
    }

  } // namespace Algorithm
} // namespace Mantid
