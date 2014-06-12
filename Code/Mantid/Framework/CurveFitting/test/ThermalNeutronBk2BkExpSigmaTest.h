#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPSIGMATEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPSIGMATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ThermalNeutronBk2BkExpSigma.h"

using Mantid::CurveFitting::ThermalNeutronBk2BkExpSigma;
using namespace Mantid;

class ThermalNeutronBk2BkExpSigmaTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronBk2BkExpSigmaTest *createSuite() { return new ThermalNeutronBk2BkExpSigmaTest(); }
  static void destroySuite( ThermalNeutronBk2BkExpSigmaTest *suite ) { delete suite; }

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
    Mantid::CurveFitting::ThermalNeutronBk2BkExpSigma function;
    function.initialize();

    function.setParameter("Sig2",  sqrt(11.380));
    function.setParameter("Sig1",   sqrt(9.901));
    function.setParameter("Sig0",  sqrt(17.370));

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


#endif /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPSIGMATEST_H_ */
