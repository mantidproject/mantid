#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPALPHATEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPALPHATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ThermalNeutronBk2BkExpAlpha.h"

using namespace Mantid;
using Mantid::CurveFitting::ThermalNeutronBk2BkExpAlpha;

class ThermalNeutronBk2BkExpAlphaTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronBk2BkExpAlphaTest *createSuite() { return new ThermalNeutronBk2BkExpAlphaTest(); }
  static void destroySuite( ThermalNeutronBk2BkExpAlphaTest *suite ) { delete suite; }


  void test_Calculation()
  {
    // 1. Input data for test
    std::vector<double> vec_d;
    std::vector<double> vec_tof;

    vec_d.push_back(2.72452);  vec_tof.push_back(62070.4);
    vec_d.push_back(2.84566);  vec_tof.push_back(64834.9);
    vec_d.push_back(3.33684);  vec_tof.push_back(76039.6);
    vec_d.push_back(4.719  );  vec_tof.push_back(107542);
    vec_d.push_back(5.44903);  vec_tof.push_back(124187);

    // 2. Initialize the method
    Mantid::CurveFitting::ThermalNeutronBk2BkExpAlpha function;
    function.initialize();

    function.setParameter("Alph0", 4.026);
    function.setParameter("Alph1", 7.362);
    function.setParameter("Alph0t", 60.683);
    function.setParameter("Alph1t", 39.730);

    function.setParameter("Width", 1.0055);
    function.setParameter("Tcross", 0.4700);

    // 3. Set up domain
    API::FunctionDomain1DVector domain(vec_d);
    API::FunctionValues values(domain);

    function.function(domain, values);

    // 4. Check result
    for (size_t i = 0; i < domain.size(); ++i)
    {
      TS_ASSERT(values[i] > 0.0);
    }

    return;
  }

};


#endif /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPALPHATEST_H_ */
