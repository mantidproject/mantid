#include <iostream>

#include "inc/LibraryManager.h"
#include "inc/AlgorithmManager.h"
#include "inc/IAlgorithm.h"

int main()
{
	
	Mantid::Kernel::AlgorithmManager* algMgr = Mantid::Kernel::AlgorithmManager::Instance();
	
	Mantid::Kernel::LibraryManager* libMgr = new Mantid::Kernel::LibraryManager;
		
	if (libMgr->OpenLibrary("MantidAlgorithms"))
	{
		Mantid::Kernel::IAlgorithm* alg = algMgr->create("HelloWorldAlgorithm");
	
		alg->execute();
	}
	
	
	std::cout << "Finished!" << std::endl;
	
}

