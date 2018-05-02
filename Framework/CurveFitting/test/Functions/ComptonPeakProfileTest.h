#ifndef MANTID_CURVEFITTING_ComptonPeakProfileTEST_H_
#define MANTID_CURVEFITTING_ComptonPeakProfileTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ComptonPeakProfile.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::Functions::ComptonPeakProfile;

class ComptonPeakProfileTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComptonPeakProfileTest *createSuite() {
    return new ComptonPeakProfileTest();
  }
  static void destroySuite(ComptonPeakProfileTest *suite) { delete suite; }

  void test_initialized_object_has_three_parameters() {
    auto profile = createFunction();
    TS_ASSERT_EQUALS(3, profile->nParams());
  }

  void test_initialized_object_has_expected_attributes() {
    auto profile = createFunction();
    static const size_t nattrs(3);
    const char *expectedAttrs[nattrs] = {"WorkspaceIndex", "Mass",
                                         "VoigtEnergyCutOff"};

    TS_ASSERT_EQUALS(nattrs, profile->nAttributes());

    // Test names as they are used in scripts
    if (profile->nAttributes() > 0) {
      std::unordered_set<std::string> expectedAttrSet(expectedAttrs,
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

  void test_function_gives_expected_value_for_given_input_data() {
    using namespace Mantid::API;

    auto peakProfile = createFunction();

    auto domain = boost::shared_ptr<FunctionDomain1DVector>(
        new FunctionDomain1DVector(-1, 1, 3));
    Mantid::API::FunctionValues outputs(*domain);
    peakProfile->setParameter(0, 0.93);
    peakProfile->setParameter(1, 0.4);
    peakProfile->setParameter(2, 4.29);

    peakProfile->function(*domain, outputs);

    TS_ASSERT_DELTA(0.14694800, outputs.getCalculated(0), 1e-08);
    TS_ASSERT_DELTA(0.34795949, outputs.getCalculated(1), 1e-08);
    TS_ASSERT_DELTA(0.31659214, outputs.getCalculated(2), 1e-08);
  }

private:
  Mantid::API::IFunction_sptr createFunction() {
    Mantid::API::IFunction_sptr profile =
        boost::make_shared<ComptonPeakProfile>();
    profile->initialize();
    auto paramWS = ComptonProfileTestHelpers::createTestWorkspace(
        1, 300, 351, 0.5, ComptonProfileTestHelpers::NoiseType::None, true,
        true); // Only using for parameters
    profile->setAttributeValue("Mass", 1.0079);
    TS_ASSERT_THROWS_NOTHING(profile->setWorkspace(paramWS));
    profile->setUpForFit();

    return profile;
  }
};

#endif /* MANTID_CURVEFITTING_COMPTONPEAKPROFILETEST_H_ */
