//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/shared_ptr.hpp>
#include <sstream>
#include <numeric>
#include <math.h>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(FindDeadDetectors)

    using namespace Kernel;
    using API::WorkspaceProperty;
    using DataObjects::Workspace2D;
    using DataObjects::Workspace2D_sptr;
    using API::Workspace;
    using API::Workspace_sptr;
    using API::LocatedDataRef;

    // Get a reference to the logger
    Logger& FindDeadDetectors::g_log = Logger::get("FindDeadDetectors");

    /** Initialisation method.
    *
    */
    void FindDeadDetectors::init()
    {
      declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("DeadThreshold",0, mustBePositive);
      // As the property takes ownership of the validator pointer, have to take care to pass in a unique
      // pointer to each property.
      declareProperty("LiveValue",0, mustBePositive->clone());
      declareProperty("DeadValue",100, mustBePositive->clone());
    }

    /** Executes the algorithm
    *
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void FindDeadDetectors::exec()
    {
      // Try and retrieve the optional properties
      int deadThreshold = getProperty("DeadThreshold");
      int liveValue = getProperty("LiveValue");
      int deadValue = getProperty("DeadValue");

      // Get the integrated input workspace 
      Workspace_sptr integratedWorkspace = integrateWorkspace(getPropertyValue("OutputWorkspace"));

      // iterate over the data values setting the live and dead values
      g_log.information() << "Marking dead detectors" << std::endl;
      for(Workspace::iterator wi(*integratedWorkspace); wi != wi.end(); ++wi)
      {
          LocatedDataRef tr = *wi;
          tr.Y() = (tr.Y() > liveValue)?liveValue:deadValue;
      }

      // Assign it to the output workspace property
      setProperty("OutputWorkspace",integratedWorkspace);

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

      Workspace2D_sptr retVal = childAlg->getProperty("OutputWorkspace");

      return boost::dynamic_pointer_cast<Workspace>(retVal);
    }

  } // namespace Algorithm
} // namespace Mantid
