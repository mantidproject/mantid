//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>

#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryManager.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

namespace Mantid
{
namespace Kernel
{
  namespace fs = boost::filesystem;  // to help clarify which bits are boost in code below

  Logger& LibraryManager::g_log = Logger::get("LibraryManager");

	/// Constructor
	LibraryManager::LibraryManager() : module(0)
	{}

	/// Destructor
	LibraryManager::~LibraryManager()
	{
		//Close lib
		if (module)
		{
			DllOpen::CloseDll(module);
			module = 0;
		}
	}

	/** Opens a DLL
	 *  @param libName The name of the file to open (not including the lib/so/dll)
	 *  @return True if DLL is opened or already open
	 */
	bool LibraryManager::OpenLibrary(const std::string& libName)
	{
		if (!module)
		{		
			//Load dynamically loaded library
			module = DllOpen::OpenDll(libName);
			if (!module) 
			{
				return false;
			}
		}
	
		return true;
	}
	
	/** Opens a DLL
	 *  @param libName The name of the file to open (not including the lib/so/dll)
	 *  @param filePath The filepath to the directory where the library lives
	 *  @return True if DLL is opened or already open
	 */
	bool LibraryManager::OpenLibrary(const std::string& libName, const std::string& filePath)
	{
		if (!module)
		{		
			//Load dynamically loaded library
			module = DllOpen::OpenDll(libName, filePath);
			if (!module) 
			{
				return false;
			}
		}
	
		return true;
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
            if (OpenLibrary(libName,filePath)) ++libCount;
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

