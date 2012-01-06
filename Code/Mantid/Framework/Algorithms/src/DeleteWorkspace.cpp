/*WIKI* 

If the input workspace exists then it is removed from Mantid.


*WIKI*/
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
    
    /// Sets documentation strings for this algorithm
    void DeleteWorkspace::initDocs()
    {
      this->setWikiSummary("Removes a workspace from memory. ");
      this->setOptionalMessage("Removes a workspace from memory.");
    }
    

    //--------------------------------------------------------------------------
    // Private member functions
    //--------------------------------------------------------------------------

    /// Initialize the algorithm properties
    void DeleteWorkspace::init()
    {
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
      dataStore.remove(wsName); // Logs if it doesn't exist
    }

  }

}
