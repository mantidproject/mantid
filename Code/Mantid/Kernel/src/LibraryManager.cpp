#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/algorithm/string.hpp"

#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Kernel
{

// to help clarify which bits are boost in code below
namespace fs = boost::filesystem;

/// Constructor
LibraryManagerImpl::LibraryManagerImpl() :
	g_log(Logger::get("LibraryManager"))
{
	std::cerr << "LibraryManager created." << std::endl;
	g_log.debug() << "LibraryManager created." << std::endl;
}

/// Destructor
LibraryManagerImpl::~LibraryManagerImpl()
{
	std::cerr << "LibraryManager destroyed." << std::endl;
}

/** Opens all suitable DLLs on a given path.
 *  \param filePath :: The filepath to the directory where the libraries are.
 *  \param isRecursive :: Whether to search subdirectories.
 *  \return The number of libraries opened.
 */
int LibraryManagerImpl::OpenAllLibraries(const std::string& filePath,
		bool isRecursive)
{
	int libCount = 0;

	//validate inputs
	if (fs::exists(filePath) )
	{
		//iteratate over the available files
		fs::directory_iterator end_itr; // default construction yields past-the-end
		for (fs::directory_iterator itr(filePath); itr != end_itr; ++itr)
		{
			if (fs::is_directory(itr->status()) )
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

					//use lower case library name for the map key
					std::string libNameLower =
							boost::algorithm::to_lower_copy(libName);

					//Check that a libray with this name has not already been loaded
					if (OpenLibs.find(libNameLower) == OpenLibs.end())
					{
						g_log.debug("Trying to open library: " + libName
								+ "...");
						//Try to open the library
						if (tmp->OpenLibrary(libName, filePath))
						{
							//Successfully opened, so add to map
							g_log.debug("Opened library: " + libName + ".\n");
							boost::shared_ptr<LibraryWrapper> pLib(tmp);
							OpenLibs.insert(std::pair< std::string, boost::shared_ptr<LibraryWrapper> >(libName, pLib) );
							++libCount;
						}
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

