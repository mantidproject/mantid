#ifndef MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/Functions/MultivariateGaussianComptonProfile.h"

#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::Functions::MultivariateGaussianComptonProfile;

class MultivariateGaussianComptonProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultivariateGaussianComptonProfileTest *createSuite() {
    return new MultivariateGaussianComptonProfileTest();
  }
  static void destroySuite(MultivariateGaussianComptonProfileTest *suite) {
    delete suite;
  }

  void test_Name_Is_As_Expected() {
    // These are used in scripts so should not change!
    Mantid::API::IFunction_sptr profile = createFunction();
    TS_ASSERT_EQUALS("MultivariateGaussianComptonProfile", profile->name());
  }

  void test_Initialized_Function_Has_Expected_Parameters_In_Right_Order() {
    Mantid::API::IFunction_sptr profile = createFunction();
    static const size_t nparams(5);
    const char *expectedParams[nparams] = {"Mass", "Intensity", "SigmaX",
                                           "SigmaY", "SigmaZ"};
    auto currentNames = profile->getParameterNames();
    const size_t nnames = currentNames.size();
    TS_ASSERT_EQUALS(nparams, nnames);
    if (nnames == nparams) {
      for (size_t i = 0; i < nnames; ++i) {
        TS_ASSERT_EQUALS(expectedParams[i], currentNames[i]);
      }
    }
  }

  void test_Initialized_Function_Has_Expected_Attributes() {
    Mantid::API::IFunction_sptr profile = createFunction();
    static const size_t nattrs(1);
    const char *expectedAttrs[nattrs] = {"IntegrationSteps"};

    TS_ASSERT_EQUALS(nattrs, profile->nAttributes());

    // Test names as they are used in scripts
    if (profile->nAttributes() > 0) {
      std::set<std::string> expectedAttrSet(expectedAttrs,
                                            expectedAttrs + nattrs);
      std::vector<std::string> actualNames = profile->getAttributeNames();

      for (size_t i = 0; i < nattrs; ++i) {
        const std::string &name = actualNames[i];
        size_t keyCount = expectedAttrSet.count(name);
        TSM_ASSERT_EQUALS("Expected " + name +
                              " to be found as attribute but it was not.",
                          1, keyCount);
      }
    }
  }

  void test_Expected_Results_Returned_Given_Data() {
    using namespace Mantid::API;

    auto func = createFunctionWithParamsSet();
    double x0(200.0), x1(220.0), dx(10.0);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(
        1, x0, x1, dx, ComptonProfileTestHelpers::NoiseType::None);
    auto &dataX = testWS->dataX(0);
    std::transform(
        dataX.begin(), dataX.end(), dataX.begin(),
        std::bind2nd(std::multiplies<double>(), 1e-06)); // to seconds
    func->setMatrixWorkspace(testWS, 0, dataX.front(), dataX.back());
    FunctionDomain1DView domain(dataX.data(), dataX.size());
    FunctionValues values(domain);

    TS_ASSERT_THROWS_NOTHING(func->function(domain, values));

    const double tol(1e-6);
    TS_ASSERT_DELTA(0.1777, values.getCalculated(0), tol);
    TS_ASSERT_DELTA(0.115784, values.getCalculated(1), tol);
    TS_ASSERT_DELTA(0.0730074, values.getCalculated(2), tol);
  }

  void test_Build_S2_Cache() {
    auto func = createFunctionWithParamsSet();
    func->setAttributeValue("IntegrationSteps", 34);

    std::vector<double> s2;
    func->buildS2Cache(s2);

    TS_ASSERT_EQUALS(1225, s2.size());

    const double tol(1e-3);
    TS_ASSERT_DELTA(36.0, s2[0], tol);
    TS_ASSERT_DELTA(36.0, s2[34], tol);
    TS_ASSERT_DELTA(34.598, s2[35], tol);
    TS_ASSERT_DELTA(34.598, s2[69], tol);
  }

private:
  boost::shared_ptr<MultivariateGaussianComptonProfile>
  createFunctionWithParamsSet() {
    auto func = createFunction();
    func->setAttributeValue("IntegrationSteps", 34);
    func->setParameter("Mass", 1.0);
    func->setParameter("Intensity", 1.0);
    func->setParameter("SigmaX", 2.5);
    func->setParameter("SigmaY", 2.5);
    func->setParameter("SigmaZ", 6.0);
    func->setUpForFit();
    return func;
  }

  boost::shared_ptr<MultivariateGaussianComptonProfile> createFunction() {
    auto profile = boost::make_shared<MultivariateGaussianComptonProfile>();
    profile->initialize();
    return profile;
  }
};

#endif /* MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILETEST_H_ */
