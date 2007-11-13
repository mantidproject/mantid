#include <iostream>

#include "inc/LibraryManager.h"
#include "inc/AlgorithmManager.h"
#include "inc/IAlgorithm.h"

int main()
{
	
	Mantid::Kernel::AlgorithmManager* algMgr = Mantid::Kernel::AlgorithmManager::Instance();
	
	Mantid::Kernel::LibraryManager* libMgr = Mantid::Kernel::LibraryManager::Initialise("MantidAlgorithms");
	
	if (libMgr)
	{
		Mantid::Kernel::IAlgorithm* alg = algMgr->create("HelloWorldAlgorithm");
	
		alg->execute();
	}
	
	
	std::cout << "Finished!" << std::endl;
	
}

