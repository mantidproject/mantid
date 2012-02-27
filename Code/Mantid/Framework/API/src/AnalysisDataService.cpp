#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Strings.h"

namespace Mantid
{
  namespace API
  {

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
     * If the name already exists then this throws a std::runtime_error
     * @param name The name of the object
     * @param workspace The shared pointer to the workspace to store
     */
    void AnalysisDataServiceImpl::add( const std::string& name, const boost::shared_ptr<API::Workspace>& workspace)
    {
      verifyName(name);
      //Attach the name to the workspace
      if( workspace ) workspace->setName(name);
      Kernel::DataService<API::Workspace>::add(name, workspace);
    }

   /**
     * Overwridden addOrReplace member to attach the name to the workspace when a workspace object is added to the service.
     * This will overwrite one of the same name
     * @param name The name of the object
     * @param workspace The shared pointer to the workspace to store
     */
    void AnalysisDataServiceImpl::addOrReplace( const std::string& name, const boost::shared_ptr<API::Workspace>& workspace)
    {
      verifyName(name);

      //Attach the name to the workspace
      if( workspace ) workspace->setName(name);
      Kernel::DataService<API::Workspace>::addOrReplace(name, workspace);
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

