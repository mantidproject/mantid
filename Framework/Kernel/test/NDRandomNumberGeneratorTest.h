#ifndef NDRandomNumberGeneratorTEST_H_
#define NDRandomNumberGeneratorTEST_H_

#include "MantidKernel/NDRandomNumberGenerator.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>

#include <boost/scoped_ptr.hpp>
#include <gmock/gmock.h>

class NDRandomNumberGeneratorTest : public CxxTest::TestSuite {
private:
  // RandomNumberGenerator is an interface so provide a trivial implementation
  // for the test
  class Mock3DRandomNumberGenerator
      : public Mantid::Kernel::NDRandomNumberGenerator {
  public:
    Mock3DRandomNumberGenerator()
        : Mantid::Kernel::NDRandomNumberGenerator(3) {}
    DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_METHOD0(generateNextPoint, void());
    MOCK_METHOD0(restart, void());
    MOCK_METHOD0(save, void());
    MOCK_METHOD0(restore, void());
    DIAG_ON_SUGGEST_OVERRIDE
  };

public:
  void test_That_nextPoint_Calls_generateNextPoint_Exactly_Once() {
    using namespace Mantid::Kernel;
    Mock3DRandomNumberGenerator randImpl;
    NDRandomNumberGenerator &randGen = randImpl;

    EXPECT_CALL(randImpl, generateNextPoint()).Times(1);
    randGen.nextPoint();
    TSM_ASSERT("nextPoint was called an unexpected number of times",
               ::testing::Mock::VerifyAndClearExpectations(&randImpl));
  }

  void test_That_nextPoint_Vector_Is_Same_Size_As_Number_Of_Dimenions() {
    using namespace Mantid::Kernel;
    Mock3DRandomNumberGenerator randImpl;
    NDRandomNumberGenerator &randGen = randImpl;

    TS_ASSERT_EQUALS(randGen.nextPoint().size(), randGen.numberOfDimensions());
  }
};

#endif
