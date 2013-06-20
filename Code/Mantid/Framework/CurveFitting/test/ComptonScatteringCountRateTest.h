#ifndef MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATETEST_H_
#define MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/ComptonScatteringCountRate.h"


class ComptonScatteringCountRateTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComptonScatteringCountRateTest *createSuite() { return new ComptonScatteringCountRateTest(); }
  static void destroySuite( ComptonScatteringCountRateTest *suite ) { delete suite; }

  void test_Function_Has_Expected_Intensity_Attribute_And_No_Parameters()
  {
    auto countRate = createFunction();

    TS_ASSERT(countRate->nAttributes() > 1);
    TS_ASSERT_THROWS_NOTHING(countRate->getAttribute("IntensityConstraints"));
    TS_ASSERT_EQUALS(0, countRate->nParams());
  }

  void test_Empty_String_For_Intensity_Attribute_Throws_Error()
  {
    auto countRate = createFunction();

    TS_ASSERT_THROWS(countRate->setAttributeValue("IntensityConstraints", ""), std::invalid_argument);
  }

  void test_Incorrect_String_For_Intensity_Attribute_Throws_Error()
  {
    auto countRate = createFunction();

    TS_ASSERT_THROWS(countRate->setAttributeValue("IntensityConstraints", "Matrix"), std::invalid_argument);
  }

  void test_Single_Row_In_Intensity_Attribute_Does_Not_Throw()
  {
    auto countRate = createFunction();

    // Single row
    TS_ASSERT_THROWS_NOTHING(countRate->setAttributeValue("IntensityConstraints", "Matrix(1,4)0|1|0|4"));
  }

  void test_Multiple_Rows_In_Intensity_Attribute_Does_Not_Throw()
  {
    auto countRate = createFunction();
    // Multiple rows
    TS_ASSERT_THROWS_NOTHING(countRate->setAttributeValue("IntensityConstraints", "Matrix(2,4)0|1|0|4|0|0|2|5"));
  }

private:

  Mantid::API::IFunction_sptr createFunction()
  {
    using Mantid::CurveFitting::ComptonScatteringCountRate;

    auto profile = boost::make_shared<ComptonScatteringCountRate>();
    profile->initialize();
    return profile;
  }

};


#endif /* MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATETEST_H_ */
