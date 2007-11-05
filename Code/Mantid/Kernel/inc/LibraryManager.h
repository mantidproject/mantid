#ifndef LIBRARY_MANAGER__H_
#define LIBRARY_MANAGER_H_

#include <iostream>
#include <string>
#include <map>
#include "Algorithm.h"

namespace Mantid
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

}


#endif //ALGORITHM_PROVIDER_H_
