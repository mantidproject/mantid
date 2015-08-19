#ifndef MANTID_KERNEL_MDFRAMEFACTORYTEST_H_
#define MANTID_KERNEL_MDFRAMEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class MDFrameFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDFrameFactoryTest *createSuite() { return new MDFrameFactoryTest(); }
  static void destroySuite( MDFrameFactoryTest *suite ) { delete suite; }


  void test_GeneralFrameFactory()
  {
    GeneralFrameFactory factory;
    TS_ASSERT(factory.canInterpret("anything"));

    std::unique_ptr<MDFrame> product = factory.create("anything");
    TSM_ASSERT("Should be creating a GeneralFrame", dynamic_cast<GeneralFrame*>(product.get()));
  }


};


#endif /* MANTID_KERNEL_MDFRAMEFACTORYTEST_H_ */
