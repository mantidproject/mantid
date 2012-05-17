#ifndef MERSENNETWISTERTEST_H_
#define MERSENNETWISTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/MersenneTwister.h"
#include <iostream>
#include <iomanip>

using Mantid::Kernel::MersenneTwister;

class MersenneTwisterTest : public CxxTest::TestSuite
{

public:
  
  void test_That_Object_Construction_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(MersenneTwister(1));
  }

  void test_That_Next_For_Given_Seed_Returns_Same_Value()
  {
    size_t seed(212437999);
    MersenneTwister gen_1(seed), gen_2(seed);

    TS_ASSERT_EQUALS(gen_1.nextValue(), gen_2.nextValue());
  }

  void test_That_Next_For_Different_Seeds_Returns_Different_Values()
  {
    long seed_1(212437999), seed_2(247021340);
    MersenneTwister gen_1(seed_1), gen_2(seed_2);

    TS_ASSERT_DIFFERS(gen_1.nextValue(), gen_2.nextValue());    
  }

  void test_That_A_Given_Seed_Produces_Expected_Sequence()
  {
    MersenneTwister randGen(1);
    randGen.setSeed(39857239);
    assertSequenceCorrectForSeed_39857239(randGen);
  }

  void test_That_A_Reset_Gives_Same_Sequence_Again_From_Start()
  {
    MersenneTwister randGen(1);
    randGen.setSeed(39857239);
    assertSequenceCorrectForSeed_39857239(randGen);
    randGen.restart();
    assertSequenceCorrectForSeed_39857239(randGen);
  }

  
  void test_That_Default_Range_Produces_Numbers_Between_Zero_And_One()
  {
    MersenneTwister randGen(12345);
    // Test 20 numbers
    for( std::size_t i = 0; i < 20; ++i )
    {
      double r = randGen.nextValue();
      TS_ASSERT( r >= 0.0 && r <= 1.0 );
    }
  }

  void test_That_A_Given_Range_Produces_Numbers_Within_This_Range()
  {
    long seed(15423894);
    const double start(2.5), end(5.);
    MersenneTwister randGen(seed, start, end);
    // Test 20 numbers
    for( std::size_t i = 0; i < 20; ++i )
    {
      const double r = randGen.nextValue();
      TS_ASSERT(r >= start && r <= end);
    }
  }

  void test_That_nextPoint_returns_1_Value()
  {
    MersenneTwister randGen(12345);
    // Test 20 numbers
    for( std::size_t i = 0; i < 20; ++i )
    {
      const std::vector<double> point = randGen.nextPoint();
      TS_ASSERT_EQUALS(point.size(), 1);
    }
  }

private:
  void assertSequenceCorrectForSeed_39857239(MersenneTwister &randGen)
  {
    double expectedValues[10] =
    {  0.203374452656,0.597970068222,0.120683325687,0.92372657801,0.734524340136,
       0.467380537419,0.0712658402044,0.204503614921,0.487210249063,0.885743656661 };
    // Check 10 numbers
    for( std::size_t i = 0; i < 10; ++i )
    {
      TS_ASSERT_DELTA(randGen.nextValue(), expectedValues[i], 1e-12);
    }
  }

};


#endif
