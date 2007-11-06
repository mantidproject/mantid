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
typedef Algorithm* create_alg();
typedef void destroy_alg(Algorithm*);

class LibraryManager
{
public:
	virtual ~LibraryManager();
	
	static LibraryManager* Initialise(const std::string&);

	Algorithm* CreateAlgorithm(const std::string&);
	void DestroyAlgorithm(const std::string&, Algorithm*);

private:
	static LibraryManager* instance;
	static void* module;

	LibraryManager();

};

} // namespace Kernel
} // namespace Mantid

#endif //MANTID_KERNEL_ALGORITHM_PROVIDER_H_
