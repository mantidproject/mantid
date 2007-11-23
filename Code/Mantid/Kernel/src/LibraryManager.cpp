#include <iostream>

#include "MantidKernel/DllOpen.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace Kernel
{
	
	LibraryManager::LibraryManager() : module(0)
	{}

	LibraryManager::~LibraryManager()
	{
		//Close lib
		if (module)
		{
			DllOpen::CloseDll(module);
			module = 0;
		}
	}

	bool LibraryManager::OpenLibrary(const std::string& libName)
	{
		if (!module)
		{		
			//Load dynamically loaded library
			module = DllOpen::OpenDll(libName);
			if (!module) 
			{
				std::cout << "Could not open library!\n";
				return false;
			}
		}
	
		return true;
	}


} // namespace Kernel
} // namespace Mantid

