/*WIKI*

Takes a [[WorkspaceGroup]] as input and ungroups it into several workspaces.
You can perform this from the MantidPlot GUI by selecting the WorkspaceGroup and clicking "Ungroup".

*WIKI*/
#include "MantidAlgorithms/UnGroupWorkspace.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
  namespace Algorithms
  {
    
    DECLARE_ALGORITHM(UnGroupWorkspace)
    
    /// Sets documentation strings for this algorithm
    void UnGroupWorkspace::initDocs()
    {
      this->setWikiSummary("Takes a group workspace as input and ungroups the workspace. ");
      this->setOptionalMessage("Takes a group workspace as input and ungroups the workspace.");
    }
    
    
    using namespace Kernel;
    using namespace API;
    
    /// Initialisation method
    void UnGroupWorkspace::init()
    {
      const AnalysisDataServiceImpl & data_store = AnalysisDataService::Instance();
      // Get the list of workspaces in the ADS
      std::set<std::string> workspaceList = data_store.getObjectNames();
      std::set<std::string> groupWorkspaceList;
      // Not iterate over, removing all those which are not group workspaces
      std::set<std::string>::iterator it;
      for ( it = workspaceList.begin(); it != workspaceList.end(); ++it)
      {
        WorkspaceGroup_const_sptr group = boost::dynamic_pointer_cast<const WorkspaceGroup>(data_store.retrieve(*it));
        // RNT: VC returns bad pointer after erase
        //if ( !group ) workspaceList.erase(it);
        if ( group )
        {
          groupWorkspaceList.insert(*it);
        }
      }
      // Declare a text property with the list of group workspaces as its allowed values
      declareProperty("InputWorkspace","","Name of the input workspace to ungroup",boost::make_shared<StringListValidator>(groupWorkspaceList) );
    }
    
    /** Executes the algorithm
     *  @throw std::runtime_error If the selected workspace is not a group workspace
     */
    void UnGroupWorkspace::exec()
    {
      const std::string inputws = getProperty("InputWorkspace");
      AnalysisDataServiceImpl & data_store = Mantid::API::AnalysisDataService::Instance();

      // Retrieve the input workspace
      Workspace_sptr wsSptr = data_store.retrieve(inputws);
      // Try to cast it to a WorkspaceGroup
      WorkspaceGroup_sptr wsGrpSptr = boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
      // Test the cast succeeded - it always should because of ListValidator on input property
      if( !wsGrpSptr )
      {
        throw std::runtime_error("Selected Workspace is not a WorkspaceGroup");
      }

      // Notify observers that a WorkspaceGroup is about to be unrolled
      data_store.notificationCenter.postNotification(new Mantid::API::WorkspaceUnGroupingNotification(inputws, wsSptr));

      // Remove named members from the group
      auto names = wsGrpSptr->getNames();
      for(auto it = names.begin(); it != names.end(); ++it)
      {
          if ( !it->empty() ) wsGrpSptr->remove(*it);
      }

      // Now remove the WorkspaceGroup from the ADS
      data_store.remove(inputws);

    }

  }
}
