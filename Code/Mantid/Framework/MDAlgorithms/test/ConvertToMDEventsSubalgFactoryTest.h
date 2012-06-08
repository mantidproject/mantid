#ifndef MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_
#define MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

#include "MantidMDEvents/MDTransfNoQ.h"
#include "MantidMDEvents/MDTransfModQ.h"
#include "MantidMDEvents/MDTransfQ3D.h"


using namespace Mantid;
using namespace Mantid::MDEvents;


//
class ConvertToMDEventsSubalgFactoryTest : public CxxTest::TestSuite
{
public:
static ConvertToMDEventsSubalgFactoryTest *createSuite() { return new ConvertToMDEventsSubalgFactoryTest(); }
static void destroySuite(ConvertToMDEventsSubalgFactoryTest * suite) { delete suite; }    

void testInit()
{
    std::vector<std::string> keys;

    TS_ASSERT_THROWS_NOTHING(keys = MDTransfFactory::Instance().getKeys());
    // we already have three transformation defined. It can be only more in a future;
    TS_ASSERT(keys.size()>2);
}

void testWrongAlgThrows()
{
    TS_ASSERT_THROWS(MDTransfFactory::Instance().create("Non_existing_subalgorithm"),Kernel::Exception::NotFoundError);
}

void testGetAlg()
{
    MDTransf_sptr transf;

    TS_ASSERT_THROWS_NOTHING(transf=MDTransfFactory::Instance().create("CopyToMD"));
    TS_ASSERT(dynamic_cast<MDTransfNoQ *>(transf.get()));

    TS_ASSERT_THROWS_NOTHING(transf=MDTransfFactory::Instance().create("|Q|"));
    TS_ASSERT(dynamic_cast<MDTransfModQ *>(transf.get()));

    TS_ASSERT_THROWS_NOTHING(transf=MDTransfFactory::Instance().create("Q3D"));
    TS_ASSERT(dynamic_cast<MDTransfQ3D *>(transf.get()));

}

//
ConvertToMDEventsSubalgFactoryTest()
{
       API::FrameworkManager::Instance();
}

};


#endif /* MANTID_MD_CONVERT2_MDEV_SUBALGFACTORY_TEST_H_ */

