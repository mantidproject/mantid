#ifndef UNWRAPTEST_H_
#define UNWRAPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Unwrap.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;

class UnwrapTest : public CxxTest::TestSuite
{
public:
	void testName()
	{
		TS_ASSERT_EQUALS( unwrap.name(), "Unwrap" )
	}

	void testVersion()
	{
	  TS_ASSERT_EQUALS( unwrap.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( unwrap.category(), "Units" )
	}

	void testInit()
	{
	  unwrap.initialize();
	  TS_ASSERT( unwrap.isInitialized() )

    const std::vector<Property*> props = unwrap.getProperties();
    TS_ASSERT_EQUALS( props.size(), 3 )

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[0]) )

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" )
    TS_ASSERT( props[1]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[1]) )

    TS_ASSERT_EQUALS( props[2]->name(), "LRef" )
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[2]) )
	}

	void testExec()
	{
    IAlgorithm* loader = FrameworkManager::Instance().createAlgorithm("LoadRaw");
    loader->setPropertyValue("Filename", "../../../../Test/Data/osi11886.raw");

    std::string outputSpace = "toUnwrap";
    loader->setPropertyValue("OutputWorkspace", outputSpace);
    loader->execute();
    TS_ASSERT( loader->isExecuted() )

    unwrap.setPropertyValue("InputWorkspace", outputSpace);
    unwrap.setPropertyValue("OutputWorkspace", "unwrappedWS" );
    unwrap.setPropertyValue("LRef","36.0");

	  unwrap.execute();
	  TS_ASSERT( unwrap.isExecuted() )

	  // Test the frame overlapping part
	  Unwrap unwrap2;
	  unwrap2.initialize();
    unwrap2.setPropertyValue("InputWorkspace", outputSpace);
    unwrap2.setPropertyValue("OutputWorkspace", "unwrappedWS2" );
    unwrap2.setPropertyValue("LRef","40.0");

    unwrap2.execute();
    TS_ASSERT( unwrap2.isExecuted() )
	}

private:
  Unwrap unwrap;
};

#endif /*UNWRAPTEST_H_*/
