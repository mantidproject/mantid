// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILETEST_H_

#include "MantidCurveFitting/Functions/GramCharlierComptonProfile.h"
#include <cxxtest/TestSuite.h>

#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::Functions::ComptonProfile;
using Mantid::CurveFitting::Functions::GramCharlierComptonProfile;

class GramCharlierComptonProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GramCharlierComptonProfileTest *createSuite() {
    return new GramCharlierComptonProfileTest();
  }
  static void destroySuite(GramCharlierComptonProfileTest *suite) {
    delete suite;
  }

  void test_Name_Is_As_Expected() {
    // These are used in scripts so should not change!
    Mantid::API::IFunction_sptr profile = createFunction();
    TS_ASSERT_EQUALS("GramCharlierComptonProfile", profile->name());
  }

  void test_initialized_object_has_expected_attributes() {
    Mantid::API::IFunction_sptr profile = createFunction();
    checkDefaultAttrsExist(*profile);
  }

  void
  test_Default_Initialized_Function_Has_Expected_Parameters_In_Right_Order() {
    Mantid::API::IFunction_sptr profile = createFunction();
    checkDefaultAttrsExist(*profile);
  }

  void test_Setting_HermiteCoefficients_Attribute_Adds_Expected_Parameters() {
    Mantid::API::IFunction_sptr profile = createFunction();
    profile->setAttributeValue("HermiteCoeffs", "1 0 1"); // turn on C_0 & C_4

    checkDefaultAttrsExist(*profile);
    checkDefaultParametersExist(*profile);

    static const size_t npars(5);
    TS_ASSERT_EQUALS(npars, profile->nParams());

    if (npars == profile->nParams()) {
      TSM_ASSERT_THROWS_NOTHING("Function should have a C_0 parameter",
                                profile->getParameter("C_0"));
      TSM_ASSERT_THROWS("Function should not have a C_2 parameter",
                        profile->getParameter("C_2"), const std::invalid_argument &);
      TSM_ASSERT_THROWS_NOTHING("Function should have a C_4 parameter",
                                profile->getParameter("C_4"));
    }
  }

  void
  test_Function_Returns_Same_Number_Intensity_Coefficents_As_Active_Hermite_Coefficients_If_KFSE_Is_Fixed() {
    boost::shared_ptr<ComptonProfile> profile = createFunction();
    profile->setAttributeValue("HermiteCoeffs", "1 0 1"); // turn on C_0 & C_4
    profile->fix(profile->parameterIndex("FSECoeff"));

    auto intensityIndices = profile->intensityParameterIndices();
    TS_ASSERT_EQUALS(2, intensityIndices.size());
  }

  void
  test_Function_Returns_Same_Number_Intensity_Coefficents_As_Active_Hermite_Coefficients_Plus_One_If_KFSE_Is_Free() {
    boost::shared_ptr<ComptonProfile> profile = createFunction();
    profile->setAttributeValue("HermiteCoeffs", "1 0 1"); // turn on C_0 & C_4

    auto intensityIndices = profile->intensityParameterIndices();
    TS_ASSERT_EQUALS(3, intensityIndices.size());
  }

  void test_Expected_Results_Returned_Given_Data() {
    using namespace Mantid::API;

    auto func = createFunctionWithParamsSet();
    double x0(165.0), x1(166.0),
        dx(0.5); // chosen to give put us near the peak for this mass & spectrum
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

    const double tol(1e-10);
    TS_ASSERT_DELTA(0.0027169802, values.getCalculated(0), tol);
    TS_ASSERT_DELTA(0.0027279881, values.getCalculated(1), tol);
    TS_ASSERT_DELTA(0.0027315600, values.getCalculated(2), tol);
  }

private:
  Mantid::API::IFunction_sptr createFunctionWithParamsSet() {
    auto func = createFunction();
    // must be before C_0 C_4 parameter calls as they are created by this
    // attribute
    func->setAttributeValue("HermiteCoeffs", "1 0 1");

    func->setParameter("Mass", 1.0);
    func->setParameter("C_0", 21.0);
    func->setParameter("C_4", 33.0);
    func->setParameter("FSECoeff", 0.82);
    func->setParameter("Width", 5.0);
    func->setUpForFit();
    return func;
  }

  boost::shared_ptr<GramCharlierComptonProfile> createFunction() {
    auto profile = boost::make_shared<GramCharlierComptonProfile>();
    profile->initialize();
    return profile;
  }

  void checkDefaultAttrsExist(const Mantid::API::IFunction &profile) {
    static const size_t nattrs = 1;
    TS_ASSERT_LESS_THAN_EQUALS(nattrs, profile.nAttributes()); // at least
                                                               // nattrs

    // Test names as they are used in scripts
    if (nattrs <= profile.nAttributes()) {
      const char *attrAarr[nattrs] = {"HermiteCoeffs"};
      std::unordered_set<std::string> expectedAttrs(attrAarr,
                                                    attrAarr + nattrs);
      std::vector<std::string> actualNames = profile.getAttributeNames();

      for (size_t i = 0; i < nattrs; ++i) {
        const std::string &name = actualNames[i];
        size_t keyCount = expectedAttrs.count(name);
        TSM_ASSERT_EQUALS("Attribute" + name + " was found but not expected.",
                          1, keyCount);
      }
    }
  }

  void checkDefaultParametersExist(const Mantid::API::IFunction &profile) {
    static const size_t nparams(3);
    const char *expectedParams[nparams] = {"Mass", "Width", "FSECoeff"};

    auto currentNames = profile.getParameterNames();
    const size_t nnames = currentNames.size();
    TS_ASSERT_LESS_THAN_EQUALS(nparams, nnames);
    if (nnames <= nparams) {
      for (size_t i = 0; i < nnames; ++i) {
        TS_ASSERT_EQUALS(expectedParams[i], currentNames[i]);
      }
    }
  }
};

#endif /* MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILETEST_H_ */
