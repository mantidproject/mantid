#ifndef NDRandomNumberGeneratorTEST_H_
#define NDRandomNumberGeneratorTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/NDRandomNumberGenerator.h"
#include <gmock/gmock.h>

class NDRandomNumberGeneratorTest : public CxxTest::TestSuite
{
private:
  // RandomNumberGenerator is an interface so provide a trivial implementation 
  // for the test
  class Mock3DRandomNumberGenerator : public Mantid::Kernel::NDRandomNumberGenerator
  {
  public:
    Mock3DRandomNumberGenerator()
      : Mantid::Kernel::NDRandomNumberGenerator(3)
    {}
    MOCK_METHOD0(generateNextPoint, void());
    MOCK_METHOD0(restart, void());
  };

public:

  void test_That_nextPoint_Calls_generateNextPoint_Exactly_Once()
  {
    Mock3DRandomNumberGenerator randGen;
    EXPECT_CALL(randGen, generateNextPoint()).Times(1);
    randGen.nextPoint();
    TSM_ASSERT("nextPoint was called an unexpected number of times",
              ::testing::Mock::VerifyAndClearExpectations(&randGen));
  }

  void test_That_Reset_Does_Nothing()
  {
    Mock3DRandomNumberGenerator randGen;
    EXPECT_CALL(randGen, restart()).Times(1);
    randGen.restart();
    TSM_ASSERT("restart was called an unexpected number of times",
               ::testing::Mock::VerifyAndClearExpectations(&randGen));

  }

  void test_That_nextPoint_Vector_Is_Same_Size_As_Number_Of_Dimenions()
  {
    Mock3DRandomNumberGenerator randGen;
    TS_ASSERT_EQUALS(randGen.nextPoint().size(), randGen.numberOfDimensions());
  }

};



#endif
