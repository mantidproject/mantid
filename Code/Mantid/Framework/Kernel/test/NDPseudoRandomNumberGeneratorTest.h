#ifndef NDPSEUDORANDOMNUMBERGENERATORTEST_H_
#define NDPSEUDORANDOMNUMBERGENERATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/NDPseudoRandomNumberGenerator.h"
#include "MantidKernel/MersenneTwister.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using Mantid::Kernel::NDPseudoRandomNumberGenerator;
using Mantid::Kernel::PseudoRandomNumberGenerator;

class NDPseudoRandomNumberGeneratorTest : public CxxTest::TestSuite
{
public:

  void test_That_Next_Always_Returns_ND_Size_Array()
  {
    boost::shared_ptr<NDPseudoRandomNumberGenerator> ndRand =
        createTestGenerator(boost::make_shared<Mantid::Kernel::MersenneTwister>(12345));
    for(int i = 0; i < 20; ++i)
    {
      std::vector<double> point = ndRand->nextPoint();
      TS_ASSERT_EQUALS(static_cast<unsigned int>(point.size()), 3);
    }
  }

  void test_That_Restart_Is_Passed_On_Correctly()
  {
    boost::shared_ptr<NDPseudoRandomNumberGenerator> ndRand =
        createTestGenerator(boost::make_shared<Mantid::Kernel::MersenneTwister>(12345));
    std::vector<double> firstPoint = ndRand->nextPoint();
    TS_ASSERT_THROWS_NOTHING(ndRand->restart());
    std::vector<double> firstPointAfterReset = ndRand->nextPoint();
    for(size_t i = 0; i < firstPoint.size(); ++i)
    {
      TS_ASSERT_EQUALS(firstPoint[i], firstPointAfterReset[i]);
    }
  }

  void test_That_Range_Of_SingleValue_Generator_Is_Respected()
  {
    const double start(2.1), end(3.4);
    boost::shared_ptr<NDPseudoRandomNumberGenerator> ndRand =
        createTestGenerator(boost::make_shared<Mantid::Kernel::MersenneTwister>(12345,start,end));
    std::vector<double> firstPoint = ndRand->nextPoint();
    for(size_t i = 0; i < firstPoint.size(); ++i)
    {
      TS_ASSERT(firstPoint[i] >= start && firstPoint[i] <= end);
    }
  }

private:
  boost::shared_ptr<NDPseudoRandomNumberGenerator> createTestGenerator(boost::shared_ptr<PseudoRandomNumberGenerator> singleGen)
  {
    const unsigned int ndims(3);
    return boost::make_shared<NDPseudoRandomNumberGenerator>(ndims, singleGen);
  }

};



#endif //NDPSEUDORANDOMNUMBERGENERATORTEST_H_
