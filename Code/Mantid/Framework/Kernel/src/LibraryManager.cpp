#include <iostream>

#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Logger.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <boost/algorithm/string.hpp>

namespace Mantid
{
  namespace Kernel
  {

    /// Constructor
    LibraryManagerImpl::LibraryManagerImpl() 
      : g_log(Logger::get("LibraryManager"))
    {
      g_log.debug() << "LibraryManager created." << std::endl;
    }

    /// Destructor
    LibraryManagerImpl::~LibraryManagerImpl()
    {
    }

    /** Opens all suitable DLLs on a given path.
    *  @param filePath :: The filepath to the directory where the libraries are.
    *  @param isRecursive :: Whether to search subdirectories.
    *  @return The number of libraries opened.
    */
    int LibraryManagerImpl::OpenAllLibraries(const std::string& filePath,
      bool isRecursive)
    {
      g_log.debug() << "Opening all libraries in " << filePath << "\n";
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
        // Iteratate over the available files
        Poco::DirectoryIterator end_itr;
        for (Poco::DirectoryIterator itr(libPath); itr != end_itr; ++itr)
        {
          const Poco::Path & item = itr.path();
          if ( item.isDirectory() )
          {
            if (isRecursive)
            {
              libCount += OpenAllLibraries(item.toString());
            }
          }
          else
          {
            if( loadLibrary(item.toString()) ) 
            {
              ++libCount;
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

    //-------------------------------------------------------------------------
    // Private members
    //-------------------------------------------------------------------------
    /** 
    * Load a library
    * @param filepath :: The full path to a library as a string
    */
    bool LibraryManagerImpl::loadLibrary(const std::string & filepath)
    {
      // Get the name of the library.
      std::string libName = DllOpen::ConvertToLibName(Poco::Path(filepath).getFileName());
      if( libName.empty() ) return false;
      // The wrapper will unload the library when it is deleted
      boost::shared_ptr<LibraryWrapper> dlwrap(new LibraryWrapper);
      std::string libNameLower = boost::algorithm::to_lower_copy(libName);

      //Check that a libray with this name has not already been loaded
      if (OpenLibs.find(libNameLower) == OpenLibs.end())
      {
        Poco::Path directory(filepath);
        directory.makeParent();
        if (g_log.is(Poco::Message::PRIO_DEBUG))
        {
          g_log.debug() << "Trying to open library: " << libName << " from " << directory.toString() << " ...";
        }
        //Try to open the library
        if (dlwrap->OpenLibrary(libName, directory.toString()))
        {
          //Successfully opened, so add to map
          g_log.debug("Opened library: " + libName + ".\n");
          OpenLibs.insert(std::pair< std::string, boost::shared_ptr<LibraryWrapper> >(libName, dlwrap) );
          return true;
        }
        else
        {
          return false;
        }
      }
      else
      {
        g_log.debug() << libName << " already opened, skipping load\n";
      }
      return false;
    }

  } // namespace Kernel
} // namespace Mantid

