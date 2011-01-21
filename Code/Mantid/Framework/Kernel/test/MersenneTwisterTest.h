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
    TS_ASSERT_THROWS_NOTHING(MersenneTwister());
  }

  void test_That_Next_For_Given_Seed_Returns_Same_Value()
  {
    MersenneTwister gen_1, gen_2;
    long seed(212437999);
    gen_1.setSeed(seed);
    gen_2.setSeed(seed);

    TS_ASSERT_EQUALS(gen_1.next(), gen_2.next());    
  }

  void test_That_Next_For_Different_Seeds_Returns_Different_Values()
  {
    MersenneTwister gen_1, gen_2;
    long seed_1(212437999), seed_2(247021340);
    gen_1.setSeed(seed_1);
    gen_2.setSeed(seed_2);

    TS_ASSERT_DIFFERS(gen_1.next(), gen_2.next());    
  }

  void test_That_A_Given_Seed_Produces_Expected_Sequence()
  {
    MersenneTwister randGen;
    randGen.setSeed(39857239);
    double expectedValues[10] = 
    {  0.203374452656,0.597970068222,0.120683325687,0.92372657801,0.734524340136,
       0.467380537419,0.0712658402044,0.204503614921,0.487210249063,0.885743656661 };
    // Check 10 numbers
    for( std::size_t i = 0; i < 10; ++i )
    {
      TS_ASSERT_DELTA(randGen.next(), expectedValues[i], 1e-12);
    }
  }
  
  void test_That_Default_Range_Produces_Numbers_Between_Zero_And_One()
  {
    MersenneTwister randGen;
    // Test 20 numbers
    for( std::size_t i = 0; i < 20; ++i )
    {
      double r = randGen.next();
      TS_ASSERT( r >= 0.0 && r <= 1.0 );
    }
  }

  void test_That_A_Given_Range_Produces_Numbers_Within_This_Range()
  {
    const double start(2.5), end(5.);
    MersenneTwister randGen;
    randGen.setSeed(15423894);
    randGen.setRange(start,end);
    // Test 20 numbers
    for( std::size_t i = 0; i < 20; ++i )
    {
      const double r = randGen.next();
      TS_ASSERT(r >= start && r <= end);
    }

  }

};


#endif
