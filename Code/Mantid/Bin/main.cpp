#include <iostream>

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/LibraryManager.h"

using namespace Mantid::Kernel;

int main()
{
	LibraryManager* mgr = new LibraryManager;
	
	if (mgr->OpenLibrary("MantidAlgorithms", "Plugins") )
	{ 
		std::cout << "Loaded\n";
	}
	else
	{
		std::cout << "Not Loaded\n";
	}
	
	Mantid::API::FrameworkManager manager;
	
	manager.initialize();
	  
	Mantid::API::IAlgorithm* alg= manager.createAlgorithm("HelloWorldAlgorithm");
	  
	StatusCode result = alg->execute();
}

