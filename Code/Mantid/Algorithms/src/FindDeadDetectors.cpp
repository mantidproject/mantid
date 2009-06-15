//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidDataObjects/Workspace2D.h"
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
    using DataObjects::Workspace2D;

    // Get a reference to the logger
    Logger& FindDeadDetectors::g_log = Logger::get("FindDeadDetectors");

    /// Initialisation method.
    void FindDeadDetectors::init()
    {
      declareProperty(
        new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input),
        "Name of the input workspace2D" );
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "The name to use for the output workspace" );

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
      declareProperty("StartX", EMPTY_DBL(),
        "No bin with a boundary at an x value less than this will be used\n"
        "in the summation that decides if a detector is 'dead' (default: the\n"
        "start of each histogram)" );
//STEVES remember to update the wiki
      declareProperty("EndX", EMPTY_DBL(),
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
      checkAndLoadInputs();

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
        if ( y > m_deadThreshold )
        {
        y = m_liveValue;
        }
        else
        {
          ++countSpec;
          y = m_deadValue;
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
            g_log.debug() << "Dead detector: " << *it << std::endl;
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
      //pass inputed values straight to this sub-algorithm, checking must be done there
      childAlg->setPropertyValue( "InputWorkspace", getPropertyValue("InputWorkspace") );
      childAlg->setPropertyValue( "OutputWorkspace", outputWorkspaceName);
      childAlg->setPropertyValue( "Range_lower",  getPropertyValue("StartX") );
      childAlg->setPropertyValue( "Range_upper", getPropertyValue("EndX") );

      // Now execute the sub-algorithm. Catch and log any error
      try
      {
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

    void FindDeadDetectors::checkAndLoadInputs()
    {
      // Try and retrieve the optional properties
      m_deadThreshold = getProperty("DeadThreshold");
      m_liveValue = getProperty("LiveValue");
      m_deadValue = getProperty("DeadValue");
      //more checking of StartX and EndX is done when it is passed to SimpleIngegration but there is some checking needed here
      double startX = getProperty("StartX");
      //a very low user startX value will cause SimpleIntegration to disregard the value
      if ( std::abs(startX) < 1e-7 )
      {//as this might not have been intended log it
        g_log.information("Low value of StartX, " + getPropertyValue("StartX") + ", disregarded the integration will be from the start of each spectrium");
      }
      //check if no value was set
      if ( isEmpty(startX) )
      {//it wasn't set use the dummy value that causes SimpleIntegration to use the start of the range
        setPropertyValue("StartX", "0.0");
      }
      
      double endX = getProperty("EndX");
      //look for a very low user EndX value
      if ( std::abs(endX) < 1e-7 )
      {//as this will cause SimpleIntegration to disregard the value user value
        g_log.information("Low value of EndX, " + getPropertyValue("EndX") + ", disregarded the integration will continue to the end of each spectrium");
      }
      //if no value was set
      if ( isEmpty(endX) )
      {//use the dummy value that causes SimpleIntegration do the default behavour
        setPropertyValue("EndX", "0.0");
      }
    }


  } // namespace Algorithm
} // namespace Mantid
