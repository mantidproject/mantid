#ifndef SOBOLSEQUENCETEST_H_
#define SOBOLSEQUENCETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/SobolSequence.h"
#include <iostream>
#include <iomanip>

using Mantid::Kernel::SobolSequence;

class SobolSequenceTest : public CxxTest::TestSuite
{

public:
  
  void test_That_Object_Construction_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(SobolSequence(1));
  }

  void test_That_Next_For_Two_Generators_Returns_Same_Value()
  {
    SobolSequence gen_1(3), gen_2(3);
    const std::vector<double> seq1 = gen_1.nextPoint();
    const std::vector<double> seq2 = gen_2.nextPoint();
    assertVectorValuesEqual(seq1,seq2);
  }

   void test_That_A_Given_Seed_Produces_Expected_Sequence()
   {
     SobolSequence randGen(5);
     double expectedValues[3][5] = \
     {
      {0.5,0.5,0.5,0.5,0.5},
      {0.75,0.25,0.75,0.25,0.75},
      {0.25,0.75,0.25,0.75,0.25},
     };
     // Check 10 numbers
     for( std::size_t i = 0; i < 3; ++i )
     {
       const std::vector<double> randPoint = randGen.nextPoint();
       for( std::size_t j = 0; j < 5; ++j)
       {
         TS_ASSERT_DELTA(randPoint[j], expectedValues[i][j], 1e-12);
       }
     }
   }

private:
  void assertVectorValuesEqual(const std::vector<double> lhs, const std::vector<double> rhs)
  {
    TS_ASSERT_EQUALS(lhs.size(), rhs.size());
    assertVectorValuesOp(lhs, rhs, true);
  }

  void assertVectorValuesDiffer(const std::vector<double> lhs, const std::vector<double> rhs)
  {
    return assertVectorValuesOp(lhs, rhs, false);
  }

  void assertVectorValuesOp(const std::vector<double> lhs, const std::vector<double> rhs, const bool same)
  {
    for(size_t i = 0; i < lhs.size(); ++i)
    {
      if(same) 
      {
        TS_ASSERT_EQUALS(lhs[i], rhs[i]);
      }
      else 
      {
        TS_ASSERT_DIFFERS(lhs[i], rhs[i]);
      }
    }
  }

};

class SobolSequenceTestPerformance : public CxxTest::TestSuite
{
public:

  void test_Large_Number_Of_Next_Point_Calls()
  {
    const unsigned int ndimensions(14);
    SobolSequence generator(ndimensions);
    const size_t ncalls = 10000000;
    size_t sumSizes(0); // Make sure the optimizer actuall does the loop
    for(size_t i = 0; i < ncalls; ++i)
    {
      const std::vector<double> & point = generator.nextPoint();
      sumSizes += point.size();
    }
    TS_ASSERT(sumSizes > 0);
  }
};


#endif
