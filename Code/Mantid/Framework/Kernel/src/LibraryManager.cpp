#include <iostream>

#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Logger.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <boost/algorithm/string.hpp>

namespace Mantid
{
  namespace Kernel
  {

/// Constructor
    LibraryManagerImpl::LibraryManagerImpl() :
      g_log(Logger::get("LibraryManager"))
    {
      g_log.debug() << "LibraryManager created." << std::endl;
    }

/// Destructor
    LibraryManagerImpl::~LibraryManagerImpl()
    {
      //std::cerr << "LibraryManager destroyed." << std::endl;
    }

/** Opens all suitable DLLs on a given path.
 *  @param filePath :: The filepath to the directory where the libraries are.
 *  @param isRecursive :: Whether to search subdirectories.
 *  @return The number of libraries opened.
 */
    int LibraryManagerImpl::OpenAllLibraries(const std::string& filePath,
					     bool isRecursive)
    {
      int libCount = 0;
	  
      //validate inputs
      Poco::File libPath;
      try
      {
	libPath = Poco::File(filePath);
      }
      catch(...)
      {
	return libCount;
      }
      if ( libPath.exists() && libPath.isDirectory() )
      {

        DllOpen::addSearchDirectory(filePath);
	
	//iteratate over the available files
	Poco::DirectoryIterator end_itr;
	for (Poco::DirectoryIterator itr(libPath); itr != end_itr; ++itr)
	{
	  if ( Poco::Path(itr->path()).isDirectory() )
	  {
	    if (isRecursive)
	    {
	      libCount += OpenAllLibraries(itr->path());
	    }
	  }
	  else
	  {
	    //if they are libraries
	    std::string libName = DllOpen::ConvertToLibName(Poco::Path(itr->path()).getFileName());
	    if( libName.empty() ) continue;

	    //load them
	    boost::shared_ptr<LibraryWrapper> dlwrap(new LibraryWrapper);

	    //use lower case library name for the map key
	    std::string libNameLower = boost::algorithm::to_lower_copy(libName);

	    //Check that a libray with this name has not already been loaded
	    if (OpenLibs.find(libNameLower) == OpenLibs.end())
	    {
	      g_log.debug("Trying to open library: " + libName + "...");
	      //Try to open the library
	      if (dlwrap->OpenLibrary(libName, filePath))
	      {
		//Successfully opened, so add to map
		g_log.debug("Opened library: " + libName + ".\n");
		OpenLibs.insert(std::pair< std::string, boost::shared_ptr<LibraryWrapper> >(libName, dlwrap) );
		  ++libCount;
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

