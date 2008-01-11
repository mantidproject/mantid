//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/DllOpen.h"

namespace Mantid
{
namespace Kernel
{
	namespace fs = boost::filesystem;  // to help clarify which bits are boost in code below

	LibraryManager* LibraryManager::m_instance = 0;
	
	Logger& LibraryManager::g_log = Logger::get("LibraryManager");
	
	/// Get an instance of LibraryManager if it already exists, else creates a new object.
	LibraryManager* LibraryManager::Instance()
	{
		if (!m_instance) m_instance=new LibraryManager;
		return m_instance;
	}

	/// Constructor
	LibraryManager::LibraryManager()
	{}

	/// Destructor
	LibraryManager::~LibraryManager()
	{
		//Clear the map - will automatically call the deconstructor for
		//the items in it.
		OpenLibs.clear();
	}

  /** Opens all suitable DLLs on a given path
	 *  @param filePath The filepath to the directory where the libraries live
	 *  @param isRecursive whether to search subdirectories
	 *  @return The number of libraries opened
	 */
	int LibraryManager::OpenAllLibraries(const std::string& filePath, bool isRecursive)
  {
    int libCount = 0;

    //validate inputs
    if ( fs::exists( filePath ) )
    {
      //iteratate over the available files
      fs::directory_iterator end_itr; // default construction yields past-the-end
      for ( fs::directory_iterator itr( filePath );
          itr != end_itr;
          ++itr )
      {
        if ( fs::is_directory(itr->status()) )
        {
          if (isRecursive)
          {
            libCount += OpenAllLibraries(itr->path().string());
          }
        }
        else
        {
          //if they are libraries
          std::string libName = DllOpen::ConvertToLibName(itr->path().leaf());
          if (libName != "")
          {
		//load them
		LibraryWrapper* tmp = new LibraryWrapper;
	  
            if (tmp->OpenLibrary(libName,filePath))
	    {		
		    ++libCount;
		    boost::shared_ptr<LibraryWrapper> pLib(tmp);
		    OpenLibs.insert ( std::pair< std::string, boost::shared_ptr<LibraryWrapper> >(libName, pLib) );
	    }
          }
        }
      }
    }
    else
    {
      g_log.error("In OpenAllLibraries: " + filePath + " must be a directory."); 
    }
   
    return libCount;
  }


} // namespace Kernel
} // namespace Mantid

