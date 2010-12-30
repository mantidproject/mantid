#ifndef MANTID_LIBRARYMANAGERTEST_H_
#define MANTID_LIBRARYMANAGERTEST_H_

#include <iostream>
#include <string>
#include <vector>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid::Kernel;

class LibraryManagerTest : public CxxTest::TestSuite
{

public:
	LibraryManagerTest()
	{
		ConfigService::Instance();
	}

	void testOpenLibrary()
	{
		TS_ASSERT_LESS_THAN(0, LibraryManager::Instance().OpenAllLibraries("../../bin", false));
	}

	void testLoadedAlgorithm()
	{
		try
		{
			Mantid::API::FrameworkManager::Instance().createAlgorithm("HelloWorldAlgorithm");

			TS_ASSERT_THROWS_NOTHING(Mantid::API::FrameworkManager::Instance().exec("HelloWorldAlgorithm", ""));
		}
		catch (...)
		{
			//Probably failed because testOpenLibrary failed!
			TS_ASSERT( false );
		}
	}

};

#endif /*MANTID_LIBRARYMANAGERTEST_H_*/

