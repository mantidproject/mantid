#ifndef MANTID_KERNEL_LIBRARY_MANAGER__H_
#define MANTID_KERNEL_LIBRARY_MANAGER_H_

#include <iostream>
#include <string>
#include <map>
#include "Algorithm.h"

namespace Mantid
{
namespace Kernel
{

// the types of the class factories
//typedef Algorithm* create_alg();
//typedef void destroy_alg(Algorithm*);

class LibraryManager
{
public:
	LibraryManager();
	virtual ~LibraryManager();
	
	//Returns true if DLL is opened or already open
	bool OpenLibrary(const std::string&);

	//Algorithm* CreateAlgorithm(const std::string&);
	//void DestroyAlgorithm(const std::string&, Algorithm*);

private:
	void* module;
};

} // namespace Kernel
} // namespace Mantid

#endif //MANTID_KERNEL_ALGORITHM_PROVIDER_H_
