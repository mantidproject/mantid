#ifndef MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>

#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using Mantid::CurveFitting::ThermalNeutronDtoTOFFunction;
using namespace Mantid;
using namespace Mantid::API;

class ThermalNeutronDtoTOFFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronDtoTOFFunctionTest *createSuite() { return new ThermalNeutronDtoTOFFunctionTest(); }
  static void destroySuite( ThermalNeutronDtoTOFFunctionTest *suite ) { delete suite; }


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
      Mantid::CurveFitting::ThermalNeutronDtoTOFFunction function;
      function.initialize();

      function.setParameter("Dtt1", 22777.1);
      function.setParameter("Dtt1t", 22785.4);
      function.setParameter("Dtt2t", 0.3);
      function.setParameter("Tcross", 0.25);
      function.setParameter("Width", 5.8675);
      function.setParameter("Zero", 0.0);
      function.setParameter("Zerot", 62.5);

      // 3. Set up domain
      API::FunctionDomain1DVector domain(vec_d);
      API::FunctionValues values(domain);

      function.function(domain, values);

      // 4. Check result
      for (size_t i = 0; i < domain.size(); ++i)
      {
          std::cout << "d = " << domain[i] << ", TOF = " << values[i] << "  vs.  observed TOF = " << vec_tof[i] << std::endl;
          TS_ASSERT_DELTA(values[i], vec_tof[i], 10.0);
      }


  }

};


#endif /* MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTIONTEST_H_ */
