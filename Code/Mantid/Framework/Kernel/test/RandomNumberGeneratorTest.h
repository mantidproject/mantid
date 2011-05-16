#ifndef RANDOMNUMBERGENERATORTEST_H_
#define RANDOMNUMBERGENERATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/RandomNumberGenerator.h"

class RandomNumberGeneratorTest : public CxxTest::TestSuite
{

private:
  // RandomNumberGenerator is an interface so provide a trivial implementation 
  // for the test
  class FakeRandomNumberGenerator : public Mantid::Kernel::RandomNumberGenerator
  {
  public:
    void setSeed(size_t seedValue)
    {
      UNUSED_ARG(seedValue);
    }
    void setRange(double start, double end)
    {
      UNUSED_ARG(start); UNUSED_ARG(end);
    }
    double next()
    {
      return 0.0;
    }
  };

public:

  void test_That_Next_Returns_Zero()
  {
    FakeRandomNumberGenerator rand_gen;
    TS_ASSERT_EQUALS(rand_gen.next(), 0.0);
  }

  void test_That_SetSeed_Does_Not_Throw()
  {
    FakeRandomNumberGenerator rand_gen;
    TS_ASSERT_THROWS_NOTHING(rand_gen.setSeed(10239048));
  }

  void test_That_Set_Range_Does_Not_Throw()
  {
    FakeRandomNumberGenerator rand_gen;
    TS_ASSERT_THROWS_NOTHING(rand_gen.setRange(0.,1.0));
  }

};



#endif
