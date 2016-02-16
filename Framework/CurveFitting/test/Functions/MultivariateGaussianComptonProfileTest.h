#ifndef MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/Functions/MultivariateGaussianComptonProfile.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::Functions::MultivariateGaussianComptonProfile;
using Mantid::CurveFitting::Functions::ComptonProfile;

class MutivariateGaussianComptonProfile : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MutivariateGaussianComptonProfile *createSuite() {
    return new MutivariateGaussianComptonProfile();
  }
  static void destroySuite(MutivariateGaussianComptonProfile *suite) {
    delete suite;
  }

  void test_Name_Is_As_Expected() {
    // These are used in scripts so should not change!
    Mantid::API::IFunction_sptr profile = createFunction();
    TS_ASSERT_EQUALS("MultivariateGaussianComptonProfile", profile->name());
  }

  // TODO

private:
  boost::shared_ptr<MultivariateGaussianComptonProfile>
  createFunctionWithParamsSet() {
    auto func = createFunction();
    // TODO
    func->setUpForFit();
    return func;
  }

  boost::shared_ptr<MultivariateGaussianComptonProfile> createFunction() {
    auto profile = boost::make_shared<MultivariateGaussianComptonProfile>();
    profile->initialize();
    return profile;
  }
};

#endif /* MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_ */
