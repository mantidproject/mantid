#ifndef MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTIONTEST_H_

#include <array>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Functions/ThermalNeutronDtoTOFFunction.h"

using Mantid::CurveFitting::Functions::ThermalNeutronDtoTOFFunction;
using namespace Mantid;
using namespace Mantid::API;

using namespace std;

class ThermalNeutronDtoTOFFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronDtoTOFFunctionTest *createSuite() {
    return new ThermalNeutronDtoTOFFunctionTest();
  }
  static void destroySuite(ThermalNeutronDtoTOFFunctionTest *suite) {
    delete suite;
  }

  void test_Calculation() {
    // 1. Input data for test
    std::vector<double> vec_d = {2.72452, 2.84566, 3.33684, 4.719, 5.44903};
    std::array<double, 5> vec_tof = {
        {62070.4, 64834.9, 76039.6, 107542, 124187}};

    // 2. Initialize the method
    ThermalNeutronDtoTOFFunction function;
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
    for (size_t i = 0; i < domain.size(); ++i) {
      std::cout << "d = " << domain[i] << ", TOF = " << values[i]
                << "  vs.  observed TOF = " << vec_tof[i] << '\n';
      TS_ASSERT_DELTA(values[i], vec_tof[i], 10.0);
    }

    return;
  }

  /** Test some math functions
  void P_test_math()
  {
    // 1. erfc() and its derivative
    stringstream outss;
    vector<double> xvec, erfcy, derfc, nderfc;

    for (double x = -10; x < 10; x+=0.01)
    {
      double a = erfc(x);
      double da = -2*exp(-x*x)/sqrt(M_PI);
      xvec.push_back(x);
      erfcy.push_back(a);
      derfc.push_back(da);
      outss << x <<"\t\t" << a << '\n';
    }

    // numerical derivative for erfc
    nderfc.push_back(0.0);
    for (size_t i = 1; i < xvec.size(); ++i)
    {
      double nda = (erfcy[i]-erfcy[i-1])/0.01;
      nderfc.push_back(nda);
    }

    ofstream outfile;
    outfile.open("erfc_plot.dat");
    outfile << outss.str();
    outfile.close();

    outfile.open("deriv_erfc.dat");
    for (size_t i = 0; i < xvec.size(); ++i)
      outfile << xvec[i] << "\t\t" << derfc[i] << "\t\t" << nderfc[i] << '\n';
    outfile.close();

    // 2. d~n, width~n, tcross~n: using old Bank 7 data
    // d ~ n
    double width = 5.8675;
    double tcross = 0.25;

    outfile.open("d_n_plot.dat");
    for (double d = 0.4; d < 2.5; d+=0.01)
    {
      double n = 0.5*erfc(width*(tcross-1/d));
      outfile << d << "\t\t" << n << '\n';
    }
    outfile.close();

    // width ~ n
    double d = 1.0;
    outfile.open("width_n_plot.dat");
    for (width = 0.5; width < 10.0; width += 0.05)
    {
      double n = 0.5*erfc(width*(tcross-1/d));
      outfile << width << "\t\t" << n << '\n';
    }
    outfile.close();

    width = 5.8675;
    outfile.open("tcross_n_plot.dat");
    for (tcross = -0.1; tcross < 2.0; tcross += 0.05)
    {
      double n = 0.5*erfc(width*(tcross-1/d));
      outfile << tcross << "\t\t" << n << '\n';
    }
    outfile.close();

    // 3. Epithermal neutron vs. Thermal neutron
    //    No need to worry... Thermal neutorn has some non-linear effect.

  }
  */
};

#endif /* MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTIONTEST_H_ */
