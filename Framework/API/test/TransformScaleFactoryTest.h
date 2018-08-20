#ifndef TRANSFORMSCALEFACTORYTEST_H_
#define TRANSFORMSCALEFACTORYTEST_H_

#include "ITransformScale.h"
#include "MantidAPI/TransformScaleFactory.h"
#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

class TransformScaleFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TransformScaleFactoryTest *createSuite() {
    return new TransformScaleFactoryTest();
  }
  static void destroySuite(TransformScaleFactoryTest *suite) { delete suite; }

  // Check that we can successfully create all registered class
  void test_create() {
    ITransformScale_sptr l;
    std::vector<std::string> scales =
        TransformScaleFactory::Instance().getKeys();
    for (auto it = scales.begin(); it != scales.end(); it++) {
      TS_ASSERT_THROWS_NOTHING(
          l = TransformScaleFactory::Instance().create(*it));
    }
  }

private:
  TransformScaleFactoryImpl &factory;
};

#endif /* TRANSFORMSCALEFACTORYTEST_H_ */
