#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Strings.h"

namespace Mantid
{
  namespace API
  {

    //-------------------------------------------------------------------------
    // Nested class methods
    //-------------------------------------------------------------------------
    /**
     * Constructor.
     * @param name :: The name of a workspace group.
     */
    AnalysisDataServiceImpl::GroupUpdatedNotification::GroupUpdatedNotification(const std::string& name) : 
      DataServiceNotification( name, AnalysisDataService::Instance().retrieve( name ) )
    {
    }
    /**
     * Returns the workspace pointer cast to WorkspaceGroup
     */
    boost::shared_ptr<const WorkspaceGroup> AnalysisDataServiceImpl::GroupUpdatedNotification::getWorkspaceGroup() const
    {
      return boost::dynamic_pointer_cast<const WorkspaceGroup>( this->object() );
    }

    //-------------------------------------------------------------------------
    // Public methods
    //-------------------------------------------------------------------------
    /**
    * Is the given name a valid name for an object in the ADS
    * @param name A string containing a possible name for an object in the ADS
    * @return An empty string if the name is valid or an error message stating the problem 
    * if the name is unacceptable.
    */
    const std::string AnalysisDataServiceImpl::isValid(const std::string & name) const
    {
      std::string error("");
      const std::string & illegal = illegalCharacters();
      if( illegal.empty() ) return error; //Quick route out.
      const size_t length = name.size();
      for(size_t i = 0; i < length; ++i)
      {
        if( illegal.find_first_of(name[i]) != std::string::npos )
        {
          std::ostringstream strm;
          strm << "Invalid object name '" << name  << "'. Names cannot contain any of the following characters: " << illegal;
          error = strm.str();
          break;
        }
      }
      return error;
    }

    /**
     * Overwridden add member to attach the name to the workspace when a workspace object is added to the service
     * If the name already exists then this throws a std::runtime_error. If a workspace group is added adds the
     * members which are not in the ADS yet.
     * @param name The name of the object
     * @param workspace The shared pointer to the workspace to store
     */
    void AnalysisDataServiceImpl::add( const std::string& name, const boost::shared_ptr<API::Workspace>& workspace)
    {
      verifyName(name);
      //Attach the name to the workspace
      if( workspace ) workspace->setName(name);
      Kernel::DataService<API::Workspace>::add(name, workspace);
      
      // if a group is added add its members as well
      auto group = boost::dynamic_pointer_cast<WorkspaceGroup>( workspace );
      if ( !group ) return;
      for(size_t i = 0; i < group->size(); ++i)
      {
        auto ws = group->getItem( i );
        std::string wsName = ws->name();
        // if anonymous make up a name and add
        if ( wsName.empty() )
        {
          wsName = name + "_" + boost::lexical_cast<std::string>( i + 1 );
          ws->setName( wsName );
        }
      }
    }

   /**
     * Overwridden addOrReplace member to attach the name to the workspace when a workspace object is added to the service.
     * This will overwrite one of the same name. If the workspace is group adds or replaces its members.
     * @param name The name of the object
     * @param workspace The shared pointer to the workspace to store
     */
    void AnalysisDataServiceImpl::addOrReplace( const std::string& name, const boost::shared_ptr<API::Workspace>& workspace)
    {
      verifyName(name);

      //Attach the name to the workspace
      if( workspace ) workspace->setName(name);
      Kernel::DataService<API::Workspace>::addOrReplace(name, workspace);

      // if a group is added add its members as well
      auto group = boost::dynamic_pointer_cast<WorkspaceGroup>( workspace );
      if ( !group ) return;
      for(size_t i = 0; i < group->size(); ++i)
      {
        auto ws = group->getItem( i );
        std::string wsName = ws->name();
        // make up a name for an anonymous workspace
        if ( wsName.empty() )
        {
          wsName = name + "_" + boost::lexical_cast<std::string>( i + 1 );
        }
      }
    }

    /**
     * Overridden rename member to attach the new name to the workspace when a workspace object is renamed
     * @param oldName The old name of the object
     * @param newName The new name of the object
     */
    void AnalysisDataServiceImpl::rename( const std::string& oldName, const std::string& newName)
    {
      Kernel::DataService<API::Workspace>::rename( oldName, newName );
      //Attach the new name to the workspace
      auto ws = retrieve( newName );
      ws->setName( newName );
    }

    /**
      * Extend the default behaviour by searching workspace groups recursively.
      * @param name :: Name of the workspace.
      */
    boost::shared_ptr<API::Workspace> AnalysisDataServiceImpl::retrieve(const std::string &name) const
    {
        std::vector<Workspace_sptr> workspaces = getObjects();
        for(auto it = workspaces.begin(); it != workspaces.end(); ++it)
        {
          Workspace *ws = it->get();
          if ( ws->name() == name ) return *it;
          WorkspaceGroup* wsg = dynamic_cast<WorkspaceGroup*>(ws);
          if ( wsg )
          {
              // look in member groups recursively
              auto res = wsg->findItem(name);
              if ( res ) return res;
          }
        }
        throw Kernel::Exception::NotFoundError("Workspace",name);
    }

    //-------------------------------------------------------------------------
    // Private methods
    //-------------------------------------------------------------------------

    /**
     * Constructor
     */
    AnalysisDataServiceImpl::AnalysisDataServiceImpl()
      :Mantid::Kernel::DataService<Mantid::API::Workspace>("AnalysisDataService"), m_illegalChars()
    {
    }

    /**
     * Destructor
     */
    AnalysisDataServiceImpl::~AnalysisDataServiceImpl()
    {

    }


    // The following is commented using /// rather than /** to stop the compiler complaining
    // about the special characters in the comment fields.
    /// Return a string containing the characters not allowed in names objects within ADS
    /// @returns A n array of c strings containing the following characters: " +-/*\%<>&|^~=!@()[]{},:.`$?"
    const std::string & AnalysisDataServiceImpl::illegalCharacters() const
    {
      return m_illegalChars;
    }

    /**
     * Set the list of illegal characeters
     * @param illegalChars A string containing the characters, as one long string, that are not to be accepted by the ADS
     * NOTE: This only affects further additions to the ADS
     */
    void AnalysisDataServiceImpl::setIllegalCharacterList(const std::string & illegalChars)
    {
      m_illegalChars = illegalChars;
    }


    /**
     * Checks the name is valid
     * @param name A string containing the name to check. If the name is invalid a std::invalid_argument is thrown
     */
    void AnalysisDataServiceImpl::verifyName(const std::string & name)
    {
      const std::string error = isValid(name);
      if( !error.empty() )
      {
        throw std::invalid_argument(error);
      }
    }


  } // Namespace API
} // Namespace Mantid

