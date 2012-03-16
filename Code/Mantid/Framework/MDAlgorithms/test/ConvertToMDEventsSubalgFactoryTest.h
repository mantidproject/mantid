#ifndef MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_
#define MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

#include "MantidMDAlgorithms/ConvertToMDEventsSubalgFactory.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;


//
class ConvertToMDEventsSubalgFactoryTest : public CxxTest::TestSuite
{
 std::auto_ptr<ConvertToMDEventsSubalgFactory> pFact;
 std::auto_ptr<ConvertToMDEventsParams> pParams;
public:
static ConvertToMDEventsSubalgFactoryTest *createSuite() { return new ConvertToMDEventsSubalgFactoryTest(); }
static void destroySuite(ConvertToMDEventsSubalgFactoryTest * suite) { delete suite; }    

void testInit()
{
    TS_ASSERT_THROWS_NOTHING(pFact->initSubalgorithms(*pParams));
}

ConvertToMDEventsSubalgFactoryTest()
{
        pFact = std::auto_ptr<ConvertToMDEventsSubalgFactory>(new ConvertToMDEventsSubalgFactory());
        pParams=std::auto_ptr<ConvertToMDEventsParams>(new ConvertToMDEventsParams());
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

