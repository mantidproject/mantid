//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------
#include "MantidAlgorithms/DeleteWorkspace.h"

namespace Mantid
{

  namespace Algorithms
  {
    
    // Register the algorithm
    DECLARE_ALGORITHM(DeleteWorkspace);

    //--------------------------------------------------------------------------
    // Private member functions
    //--------------------------------------------------------------------------

    /// Initialize the algorithm properties
    void DeleteWorkspace::init()
    {
      this->setWikiSummary("Removes a workspace from memory.");
      this->setOptionalMessage("Removes a workspace from memory.");

      declareProperty(new API::WorkspaceProperty<API::Workspace> ("Workspace", "", 
								  Kernel::Direction::Input), "Name of the workspace to delete.");
    }

    /// Execute the algorithm 
    void DeleteWorkspace::exec()
    {
      using API::AnalysisDataService;
      using API::AnalysisDataServiceImpl; 
      AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
      const std::string wsName = getProperty("Workspace");
      // Check just in case something else has deleted it in between the property check and execute
      if( dataStore.doesExist(wsName) )
      {
        dataStore.remove(wsName);
      }
      else
      {
        g_log.warning() << "Workspace \"" << wsName << "\" does not exist in the analysis data service.\n";
      }
    }

  }

}
