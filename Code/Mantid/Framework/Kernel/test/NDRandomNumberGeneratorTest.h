#ifndef NDRandomNumberGeneratorTEST_H_
#define NDRandomNumberGeneratorTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/NDRandomNumberGenerator.h"

class NDRandomNumberGeneratorTest : public CxxTest::TestSuite
{

private:
  // RandomNumberGenerator is an interface so provide a trivial implementation 
  // for the test
  class FakeNDRandomNumberGenerator : public Mantid::Kernel::NDRandomNumberGenerator
  {
  public:
    std::vector<double> nextPoint()
    {
      return std::vector<double>();
    }
    void reset()
    {
    }
  };

public:

  void test_That_Next_Returns_Empty()
  {
    FakeNDRandomNumberGenerator rand_gen;
    std::vector<double> point = rand_gen.nextPoint();
    TS_ASSERT(point.empty());
  }

  void test_That_Reset_Does_Nothing()
  {
    FakeNDRandomNumberGenerator rand_gen;
    TS_ASSERT_THROWS_NOTHING(rand_gen.reset());
  }

};



#endif
