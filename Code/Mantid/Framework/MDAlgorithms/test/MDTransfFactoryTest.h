#ifndef MANTID_MD_CONVERT2_MDEV_FACTORY_TEST_H_
#define MANTID_MD_CONVERT2_MDEV_FACTORY_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

#include "MantidMDEvents/MDTransfNoQ.h"
#include "MantidMDEvents/MDTransfModQ.h"
#include "MantidMDEvents/MDTransfQ3D.h"


using namespace Mantid;
using namespace Mantid::MDEvents;


//
class MDTransfFactoryTest : public CxxTest::TestSuite
{
public:
static MDTransfFactoryTest *createSuite() { return new MDTransfFactoryTest(); }
static void destroySuite(MDTransfFactoryTest * suite) { delete suite; }    

void testInit()
{
    std::vector<std::string> keys;

    TS_ASSERT_THROWS_NOTHING(keys = MDTransfFactory::Instance().getKeys());
    // we already have three transformation defined. It can be only more in a future;
    TS_ASSERT(keys.size()>2);
}

void testWrongAlgThrows()
{
    TS_ASSERT_THROWS(MDTransfFactory::Instance().create("Non_existing_ChildAlgorithm"),Kernel::Exception::NotFoundError);
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
MDTransfFactoryTest()
{
       API::FrameworkManager::Instance();
}

};


#endif /* MANTID_MD_CONVERT2_MDEV_ChildAlgFACTORY_TEST_H_ */

