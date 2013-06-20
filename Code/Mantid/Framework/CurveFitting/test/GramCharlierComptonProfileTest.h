#ifndef MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/GramCharlierComptonProfile.h"

using Mantid::CurveFitting::GramCharlierComptonProfile;

class GramCharlierComptonProfileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GramCharlierComptonProfileTest *createSuite() { return new GramCharlierComptonProfileTest(); }
  static void destroySuite( GramCharlierComptonProfileTest *suite ) { delete suite; }

  void test_Name_Is_As_Expected()
  {
    // These are used in scripts so should not change!
    Mantid::API::IFunction_sptr profile = createFunction();
    TS_ASSERT_EQUALS("GramCharlierComptonProfile", profile->name());
  }

  void test_initialized_object_has_expected_attributes()
  {
    Mantid::API::IFunction_sptr profile = createFunction();
    checkDefaultAttrsExist(*profile);
  }

  void test_Default_Initialized_Function_Has_Expected_Parameters_In_Right_Order()
  {
    Mantid::API::IFunction_sptr profile = createFunction();
    checkDefaultAttrsExist(*profile);
  }

  void test_Setting_HermiteCoefficients_Attribute_Adds_Expected_Parameters()
  {
    Mantid::API::IFunction_sptr profile = createFunction();
    profile->setAttributeValue("HermiteCoeffs", "1 0 1"); // turn on C_0 & C_4

    checkDefaultAttrsExist(*profile);
    checkDefaultParametersExist(*profile);

    static const size_t npars(4);
    TS_ASSERT_EQUALS(npars, profile->nParams());

    if(npars == profile->nParams())
    {
      TSM_ASSERT_THROWS_NOTHING("Function should have a C_0 parameter",profile->getParameter("C_0"));
      TSM_ASSERT_THROWS("Function should not have a C_2 parameter", profile->getParameter("C_2"), std::invalid_argument);
      TSM_ASSERT_THROWS_NOTHING("Function should have a C_4 parameter", profile->getParameter("C_4"));
    }
  }

  void test_Function_Returns_Same_Number_Intensity_Coefficents_As_Active_Hermite_Coefficients_If_KFSE_Is_Fixed()
  {
    boost::shared_ptr<Mantid::CurveFitting::ComptonProfile> profile = createFunction();
    profile->setAttributeValue("HermiteCoeffs", "1 0 1"); // turn on C_0 & C_4
    profile->fix(profile->parameterIndex("FSECoeff"));

    auto intensityIndices = profile->intensityParameterIndices();
    TS_ASSERT_EQUALS(2, intensityIndices.size());
  }

  void test_Function_Returns_Same_Number_Intensity_Coefficents_As_Active_Hermite_Coefficients_Plus_One_If_KFSE_Is_Free()
  {
    boost::shared_ptr<Mantid::CurveFitting::ComptonProfile> profile = createFunction();
    profile->setAttributeValue("HermiteCoeffs", "1 0 1"); // turn on C_0 & C_4

    auto intensityIndices = profile->intensityParameterIndices();
    TS_ASSERT_EQUALS(3, intensityIndices.size());
  }

private:

  boost::shared_ptr<GramCharlierComptonProfile> createFunction()
  {
    auto profile = boost::make_shared<GramCharlierComptonProfile>();
    profile->initialize();
    return profile;
  }

  void checkDefaultAttrsExist(const Mantid::API::IFunction & profile)
  {
    static const size_t nattrs = 3;
    TS_ASSERT_LESS_THAN_EQUALS(nattrs, profile.nAttributes()); //at least nattrs

    // Test names as they are used in scripts
    if(nattrs <= profile.nAttributes())
    {
      const char * attrAarr[nattrs] = {"WorkspaceIndex","Mass", "HermiteCoeffs"};
      std::set<std::string> expectedAttrs(attrAarr, attrAarr + nattrs);
      std::vector<std::string> actualNames = profile.getAttributeNames();

      for(size_t i = 0; i < nattrs; ++i)
      {
        const std::string & name = actualNames[i];
        size_t keyCount = expectedAttrs.count(name);
        TSM_ASSERT_EQUALS("Attribute" + name + " was found but not expected.", 1, keyCount);
      }
    }
  }

  void checkDefaultParametersExist(const Mantid::API::IFunction & profile)
  {
    static const size_t nparams(2);
    const char * expectedParams[nparams] = {"Width", "FSECoeff"};

    auto currentNames = profile.getParameterNames();
    const size_t nnames = currentNames.size();
    TS_ASSERT_LESS_THAN_EQUALS(nparams, nnames);
    if(nnames <= nparams)
    {
      for(size_t i = 0; i < nnames; ++i)
      {
        TS_ASSERT_EQUALS(expectedParams[i], currentNames[i]);
      }
    }

  }

};


#endif /* MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILETEST_H_ */
