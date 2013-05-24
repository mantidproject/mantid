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
      if ( doesExist(name) )
      {
          std::string error="ADS : Unable to add workspace : '"+name+"'";
          throw std::runtime_error(error);
      }
      //Attach the name to the workspace
      if( workspace ) workspace->setName(name);
      Kernel::DataService<API::Workspace>::add(name, workspace);
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

      // if workspace is already in the ADS this is equivalent to rename
      if ( ! workspace->name().empty() )
      {
          if (workspace->name() != name )
          {
              rename( workspace->name(), name );
          }
          return;
      }
      //Attach the new name to the workspace
      if( workspace ) workspace->setName(name,true);
      Kernel::DataService<API::Workspace>::addOrReplace(name, workspace);
    }

    /**
     * Overridden rename member to attach the new name to the workspace when a workspace object is renamed
     * @param oldName The old name of the object
     * @param newName The new name of the object
     */
    void AnalysisDataServiceImpl::rename( const std::string& oldName, const std::string& newName)
    {
      auto ws = retrieve( oldName );
      Kernel::DataService<API::Workspace>::rename( oldName, newName );
      //Attach the new name to the workspace
      ws->setName( newName, true );
    }

    /**
      * Extend the default behaviour by searching workspace groups recursively. Search is case insensitive.
      * @param name :: Name of the workspace.
      * @throw NotFoundError if the workspace wasn't found
      */
    boost::shared_ptr<API::Workspace> AnalysisDataServiceImpl::retrieve(const std::string &name) const
    {
        auto ws = find(name);
        if ( ws ) return ws;
        throw Kernel::Exception::NotFoundError("Workspace",name);
    }

    /**
     * Search recursively in the data store and workspace groups in it for a name.
     * @param name :: Name to search for.
     * @return True if name is found.
     */
    bool AnalysisDataServiceImpl::doesExist(const std::string &name) const
    {
        auto it = find(name);
        if ( it ) return true;
        return false;
    }

    /**
     * Remove a workspace
     * @param name :: Name of a workspace to remove.
     */
    void AnalysisDataServiceImpl::remove(const std::string &name)
    {
        if ( removeFromTopLevel(name) ) return;

        std::vector<Workspace_sptr> workspaces = getObjects();
        for(auto it = workspaces.begin(); it != workspaces.end(); ++it)
        {
            WorkspaceGroup* wsg = dynamic_cast<WorkspaceGroup*>( it->get() );
            if ( wsg  )
            {
                wsg->deepRemove(name);
            }
        }
    }

    /**
     * A method to help with workspace group management.
     * @param name :: Name of a workspace to emove.
     */
    bool AnalysisDataServiceImpl::removeFromTopLevel(const std::string &name)
    {
        std::string foundName = name;
        std::transform(foundName.begin(), foundName.end(), foundName.begin(),toupper);
        std::vector<Workspace_sptr> workspaces = getObjects();
        for(auto it = workspaces.begin(); it != workspaces.end(); ++it)
        {
            if ( (**it).getUpperCaseName() == foundName )
            {
                (**it).setName("",true); // this call goes before remove(name) to work correctly with workspace groups
                Kernel::DataService<Workspace>::remove( name );
                return true;
            }
        }
        return false;
    }

    /**
     * Get number of copies of a workspace in the ADS.
     * @param workspace :: A workspace to count.
     * @return :: Number of copies.
     */
    size_t AnalysisDataServiceImpl::count(Workspace_const_sptr workspace) const
    {
        size_t n = 0;
        std::vector<Workspace_sptr> workspaces = getObjects();
        for(auto it = workspaces.begin(); it != workspaces.end(); ++it)
        {
            if ( (*it) == workspace ) n += 1;
            const Workspace *ws = it->get();
            const WorkspaceGroup* wsg = dynamic_cast<const WorkspaceGroup*>(ws);
            if ( wsg )
            {
                n += wsg->count( workspace );
            }
        }
        return n;
    }

    /**
     * Find a workspace in the ADS.
     * @param name :: Name of the workspace to find.
     * @return :: Shared pointer to the found workspace or an empty pointer otherwise.
     */
    boost::shared_ptr<Workspace> AnalysisDataServiceImpl::find(const std::string &name) const
    {
        std::string foundName = name;
        std::transform(foundName.begin(), foundName.end(), foundName.begin(),toupper);
        std::vector<Workspace_sptr> workspaces = getObjects();
        for(auto it = workspaces.begin(); it != workspaces.end(); ++it)
        {
          Workspace *ws = it->get();
          if ( ws->getUpperCaseName() == foundName ) return *it;
          WorkspaceGroup* wsg = dynamic_cast<WorkspaceGroup*>(ws);
          if ( wsg )
          {
              // look in member groups recursively
              auto res = wsg->findItem(foundName, false);
              if ( res ) return res;
          }
        }
        return boost::shared_ptr<Workspace>();
    }

    /**
     * Print the names of all the workspaces in the ADS to the logger (at debug level)
     *
     */
    void AnalysisDataServiceImpl::print() const
    {
        g_log.debug() << "Workspaces in ADS:" << std::endl;
        std::vector<Workspace_sptr> workspaces = getObjects();
        for(auto it = workspaces.begin(); it != workspaces.end(); ++it)
        {
          Workspace *ws = it->get();
          g_log.debug() << (**it).name() << std::endl;
          WorkspaceGroup* wsg = dynamic_cast<WorkspaceGroup*>(ws);
          if ( wsg )
          {
              wsg->print("  ");
          }
        }
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

