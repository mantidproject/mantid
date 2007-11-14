#include <iostream>
#include <iomanip>

#include "SimpleIntegration.h"
#include "Exception.h"
#include "FrameworkManager.h"
#include "Algorithm.h"


using namespace Mantid::Kernel;
int
main()
{
#if defined _DEBUG
	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!
	FrameworkManager manager;
	manager.initialize();

	try
	{
		manager.createAlgorithm("NotThere");
	}
	catch (Exception::NotFoundError &ex)
	{
	  std::cerr << ex.what() <<std::endl;
	}

	manager.clear();

#endif
	exit(0);
}