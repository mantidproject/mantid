#ifndef MANTID_KERNEL_HERMITEPOLYNOMIALSTEST_H_
#define MANTID_KERNEL_HERMITEPOLYNOMIALSTEST_H_

#include <cxxtest/TestSuite.h>
#include <iomanip>
#include "MantidKernel/Math/Distributions/HermitePolynomials.h"

class HermitePolynomialsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HermitePolynomialsTest *createSuite() { return new HermitePolynomialsTest(); }
  static void destroySuite( HermitePolynomialsTest *suite ) { delete suite; }

  // clang-format off
  void test_hermitePoly_With_SingleValue_Returns_Expected_Values_For_First_Few_Terms()
  // clang-format on
  {
    using namespace Mantid::Kernel;
    static const unsigned int npoly(6);

    const double xValue = 3.98753;
    const double expectedValues [npoly] = {1, 7.97506, 61.6015820036, 459.376072573630, 3293.942249317455, 22594.378494252622};

    const double tolerance(1e-12);
    for(unsigned int i = 0; i < npoly; ++i)
    {
      TS_ASSERT_DELTA(expectedValues[i], Math::hermitePoly(i, xValue), tolerance);
    }
  }

  // clang-format off
  void test_hermitePoly_With_Array_Values_Returns_Expected_Values_For_First_Few_Terms()
  // clang-format on
  {
    using namespace Mantid::Kernel;

    static const unsigned int nx(5), npoly(3);
    std::vector<double> xvalues(nx, 1.0);
    const double delta = std::sqrt(3.);
    for(size_t i = 1; i < nx; ++i)
    {
      xvalues[i] = xvalues[i-1] + delta;
    }

    const double expectedValues[npoly][nx] = {\
      {1,1,1,1,1},
      {2,5.464101615138,8.928203230276,12.392304845413,15.856406460551},
      {2,27.856406460551,77.712812921102,151.569219381653,249.425625842204},
    };
    const double tolerance(1e-12);
    for(unsigned int i = 0; i < npoly; ++i)
    {
      auto hpoly = Math::hermitePoly(i, xvalues);
      for(unsigned int j = 0; j < nx; ++j)
      {
        TS_ASSERT_DELTA(expectedValues[i][j], hpoly[j], tolerance);
      }
    }



  }

};


#endif /* MANTID_KERNEL_HERMITEPOLYNOMIALSTEST_H_ */
