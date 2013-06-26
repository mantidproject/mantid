#ifndef MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/GaussianComptonProfile.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "ComptonProfileTestHelpers.h"

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

  void test_Expected_Results_Returned_Given_Data()
  {
    using namespace Mantid::API;

    auto func = createFunctionWithParamsSet();
    double x0(370.0),x1(371.0),dx(0.5); //chosen to give put us near the peak for this mass & spectrum
    auto testWS = ComptonProfileTestHelpers::createSingleSpectrumTestWorkspace(x0,x1,dx);
    func->setWorkspace(testWS);
    const auto & dataX = testWS->readX(0);
    FunctionDomain1DView domain(dataX.data(), dataX.size());
    FunctionValues values(domain);

    TS_ASSERT_THROWS_NOTHING(func->function(domain, values));

    const double tol(1e-8);
    TS_ASSERT_DELTA(0.10489410, values.getCalculated(0), tol);
    TS_ASSERT_DELTA(0.10448893, values.getCalculated(1), tol);
    TS_ASSERT_DELTA(0.10297652, values.getCalculated(2), tol);
  }

private:

  boost::shared_ptr<GaussianComptonProfile> createFunctionWithParamsSet()
  {
    auto func = createFunction();
    func->setAttributeValue("WorkspaceIndex",0);
    func->setAttributeValue("Mass",30.0);
    func->setParameter("Intensity", 4.0);
    func->setParameter("Width", 13.0);
    func->setUpForFit();
    return func;
  }

  boost::shared_ptr<GaussianComptonProfile> createFunction()
  {
    auto profile = boost::make_shared<GaussianComptonProfile>();
    profile->initialize();
    return profile;
  }
};


#endif /* MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILETEST_H_ */
