#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/Instrument.h"
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "Component.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;
int
main()
{
#if defined _DEBUG
	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!
	FrameworkManager manager;
	manager.initialize();
	try
	{

		PropertyWithValue<int> pInt("test", 11, new BoundedValidator<int>(1,10));
		bool retValInt = pInt.isValid();

		PropertyWithValue<std::string> p("test", "", new BoundedValidator<std::string>("B","T"));
		bool retVal = p.isValid();
		p.setValue("I'm here");
		retVal = p.isValid();
		p.setValue("");
		retVal = p.isValid();
	}
	catch (Exception::NotFoundError &ex)
	{
	  std::cerr << ex.what() <<std::endl;
	}
	   
	//char c[10];
	//std::cin.getline( &c[0], 1, '2' );

	manager.clear();
#endif
	exit(0);
}
