#ifndef MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/GaussianComptonProfile.h"

using Mantid::CurveFitting::GaussianComptonProfile;

class GaussianComptonProfileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GaussianComptonProfileTest *createSuite() { return new GaussianComptonProfileTest(); }
  static void destroySuite( GaussianComptonProfileTest *suite ) { delete suite; }


  void test_Name_Is_As_Expected()
  {
    // These are used in scripts so should not change!
    Mantid::API::IFunction_sptr profile = createFunction();
    TS_ASSERT_EQUALS("GaussianComptonProfile", profile->name());
  }


  void test_Initialized_Function_Has_Expected_Parameters_In_Right_Order()
  {
    Mantid::API::IFunction_sptr profile = createFunction();
    static const size_t nparams(2);
    const char * expectedParams[nparams] = {"Width", "Intensity"};

    auto currentNames = profile->getParameterNames();
    const size_t nnames = currentNames.size();
    TS_ASSERT_EQUALS(nparams, nnames);
    if(nnames == nparams)
    {
      for(size_t i = 0; i < nnames; ++i)
      {
        TS_ASSERT_EQUALS(expectedParams[i], currentNames[i]);
      }
    }
  }

  void test_Function_Has_One_Intensity_Coefficient()
  {
    boost::shared_ptr<Mantid::CurveFitting::ComptonProfile> profile = createFunction();

    auto intensityIndices = profile->intensityParameterIndices();
    TS_ASSERT_EQUALS(1, intensityIndices.size());
  }


  void test_Initialized_Function_Has_Expected_Attributes()
  {
    Mantid::API::IFunction_sptr profile = createFunction();
    static const size_t nattrs(2);
    const char * expectedAttrs[nattrs] = {"WorkspaceIndex", "Mass"};

    TS_ASSERT_EQUALS(nattrs, profile->nAttributes());

    // Test names as they are used in scripts
    if(profile->nAttributes() > 0)
    {
      std::set<std::string> expectedAttrSet(expectedAttrs, expectedAttrs + nattrs);
      std::vector<std::string> actualNames = profile->getAttributeNames();

      for(size_t i = 0; i < nattrs; ++i)
      {
        const std::string & name = actualNames[i];
        size_t keyCount = expectedAttrSet.count(name);
        TSM_ASSERT_EQUALS("Expected " + name + " to be found as attribute but it was not.", 1, keyCount);
      }
    }
  }


private:

  boost::shared_ptr<GaussianComptonProfile> createFunction()
  {
    auto profile = boost::make_shared<GaussianComptonProfile>();
    profile->initialize();
    return profile;
  }

};


#endif /* MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_ */
