#include <iostream>

#include "DllOpen.h"
#include "Algorithm.h"
#include "LibraryManager.h"

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

	/*Algorithm* LibraryManager::CreateAlgorithm(const std::string& algName)
	{
		create_alg* createMyAlg = (create_alg*) DllOpen::GetFunction(module, algName.c_str());
	
		return createMyAlg();
	}

	void LibraryManager::DestroyAlgorithm(const std::string& algName, Algorithm* obj)
	{
		destroy_alg* destroyMyAlg = (destroy_alg*) DllOpen::GetFunction(module, algName.c_str());

		destroyMyAlg(obj);
	}*/

} // namespace Kernel
} // namespace Mantid

