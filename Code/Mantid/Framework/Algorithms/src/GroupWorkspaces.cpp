/*WIKI* 
This algorithm takes two or more workspaces as input and creates an output workspace group.
 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid
{
  namespace Algorithms
  {

    DECLARE_ALGORITHM(GroupWorkspaces);

    using namespace Kernel;
    using namespace API;

    /// Default constructor
    GroupWorkspaces::GroupWorkspaces() : API::Algorithm(), m_group(), m_firstID() 
    {
    }

    ///Initialisation method
    void GroupWorkspaces::init()
    {
      this->setWikiSummary("Takes workspaces as input and groups similar workspaces together.");
      declareProperty(new ArrayProperty<std::string> ("InputWorkspaces", boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
          "Name of the Input Workspaces to Group");
      declareProperty(new WorkspaceProperty<WorkspaceGroup> ("OutputWorkspace", "", Direction::Output),
          "Name of the workspace to be created as the output of grouping ");
    }

    /** Executes the algorithm
     *  @throw std::runtime_error If the selected workspaces are not of same types
     */
    void GroupWorkspaces::exec()
    {
      const std::vector<std::string> inputWorkspaces = getProperty("InputWorkspaces");

      m_group = boost::make_shared<WorkspaceGroup>();
      addToGroup(inputWorkspaces);

      setProperty("OutputWorkspace", m_group);
      auto & notifier = API::AnalysisDataService::Instance().notificationCenter;
      notifier.postNotification(new WorkspacesGroupedNotification(inputWorkspaces));
    }
 
    /**
     * Add a list of names to the new group
     * @param names The list of names to add from the ADS
     */
    void GroupWorkspaces::addToGroup(const std::vector<std::string> & names)
    {
      typedef std::vector<std::string>::const_iterator const_vector_iterator;

      AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();
      const_vector_iterator cend = names.end();
      for(const_vector_iterator citr = names.begin(); citr != cend; ++citr)
      {
        auto workspace = ads.retrieve(*citr);
        addToGroup(workspace);
      }
    }

    /**
     * If it is a group it is unrolled and each member added
     * @param workspace A pointer to the workspace to add
     */
    void GroupWorkspaces::addToGroup(const API::Workspace_sptr & workspace)
    {
      auto localGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
      if(localGroup)
      {
        addToGroup(localGroup->getNames());
        // Remove the group from the ADS
        AnalysisDataService::Instance().remove(workspace->name());
      }
      else
      {
        appendWSToGroup(workspace);
      }
    }

    /**
     * Append the workspace to the new group
     * @param A pointer to a single workspace. Its ID must match the others in the group
     * unless it is a TableWorkspace
     */
    void GroupWorkspaces::appendWSToGroup(const API::Workspace_sptr & workspace)
    {
      if(m_group->size() == 0) m_firstID = workspace->id();
      else if(m_firstID != workspace->id() && workspace->id() != "TableWorkspace" )
      {
        throw std::runtime_error("Selected workspaces are not all of same type. Found " + workspace->id() +
                                 " which does not match the first entry of type " + m_firstID);
      }

      /// Append
      m_group->addWorkspace(workspace);
    }
  }
}
