#ifndef MANTID_CURVEFITTING_COMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_COMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ComptonProfile.h"
#include <boost/make_shared.hpp>
#include <unordered_set>

using Mantid::CurveFitting::Functions::ComptonProfile;

class ComptonProfileTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComptonProfileTest *createSuite() { return new ComptonProfileTest(); }
  static void destroySuite(ComptonProfileTest *suite) { delete suite; }

  void test_initialized_object_has_no_attributes() {
    auto profile = createFunction();
    TS_ASSERT_EQUALS(0, profile->nAttributes());
  }

  void test_initialized_object_has_expected_parameters() {
    auto profile = createFunction();
    static const size_t nparams(1);
    const char *expectedParams[nparams] = {"Mass"};

    TS_ASSERT_EQUALS(nparams, profile->nParams());

    // Test names as they are used in scripts
    if (profile->nParams() > 0) {
      std::unordered_set<std::string> expectedParamStr(
          expectedParams, expectedParams + nparams);
      std::vector<std::string> actualNames = profile->getParameterNames();

      for (size_t i = 0; i < nparams; ++i) {
        const std::string &name = actualNames[i];
        size_t keyCount = expectedParamStr.count(name);
        TSM_ASSERT_EQUALS("Expected " + name +
                              " to be found as parameter but it was not.",
                          1, keyCount);
      }
    }
  }

private:
  struct FakeComptonProfile : ComptonProfile {
    std::string name() const override { return "FakeComptonProfile"; }
    std::vector<size_t> intensityParameterIndices() const override {
      return std::vector<size_t>();
    }
    size_t fillConstraintMatrix(
        Mantid::Kernel::DblMatrix &, const size_t,
        const Mantid::HistogramData::HistogramE &) const override {
      return 0;
    }

    void massProfile(double *, const size_t) const override {}
  };

  Mantid::API::IFunction_sptr createFunction() {
    auto profile = boost::make_shared<FakeComptonProfile>();
    profile->initialize();
    return profile;
  }
};

#endif /* MANTID_CURVEFITTING_COMPTONPROFILETEST_H_ */
