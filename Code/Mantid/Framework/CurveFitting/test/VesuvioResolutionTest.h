#ifndef MANTID_CURVEFITTING_VESUVIORESOLUTIONTEST_H_
#define MANTID_CURVEFITTING_VESUVIORESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/VesuvioResolution.h"

#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::VesuvioResolution;

class VesuvioResolutionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VesuvioResolutionTest *createSuite() { return new VesuvioResolutionTest(); }
  static void destroySuite( VesuvioResolutionTest *suite ) { delete suite; }

  void test_Name_Is_As_Expected()
  {
    // These are used in scripts so should not change!
    Mantid::API::IFunction_sptr func = createFunction();
    TS_ASSERT_EQUALS("VesuvioResolution", func->name());
  }

  void test_initialized_object_has_expected_attributes()
  {
    Mantid::API::IFunction_sptr func = createFunction();
    checkDefaultAttrsExist(*func);
  }

  void test_Expected_Results_Returned_Given_Data()
  {
    using namespace Mantid::API;

     auto func = createFunction();
     double x0(165.0),x1(166.0),dx(0.5); //chosen to give put us near the peak for this mass & spectrum
     auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
     auto & dataX = testWS->dataX(0);
     std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds
     func->setMatrixWorkspace(testWS,0,dataX.front(),dataX.back());
     FunctionDomain1DView domain(dataX.data(), dataX.size());
     FunctionValues values(domain);

     TS_ASSERT_THROWS_NOTHING(func->function(domain, values));

     const double tol(1e-6);
     TS_ASSERT_DELTA(0.279933, values.getCalculated(0), tol);
     TS_ASSERT_DELTA(0.279933, values.getCalculated(1), tol);
     TS_ASSERT_DELTA(0.279933, values.getCalculated(2), tol);
  }

private:

  boost::shared_ptr<VesuvioResolution> createFunction()
  {
    auto func = boost::make_shared<VesuvioResolution>();
    func->initialize();
    func->setAttributeValue("Mass",1.0);
    func->setUpForFit();
    return func;
  }

  void checkDefaultAttrsExist(const Mantid::API::IFunction & func)
  {
    static const size_t nattrs = 1;
    TS_ASSERT_LESS_THAN_EQUALS(nattrs, func.nAttributes()); //at least nattrs

    // Test names as they are used in scripts
    if(nattrs <= func.nAttributes())
    {
      const char * attrAarr[nattrs] = {"Mass"};
      std::set<std::string> expectedAttrs(attrAarr, attrAarr + nattrs);
      std::vector<std::string> actualNames = func.getAttributeNames();

      for(size_t i = 0; i < nattrs; ++i)
      {
        const std::string & name = actualNames[i];
        size_t keyCount = expectedAttrs.count(name);
        TSM_ASSERT_EQUALS("Attribute" + name + " was found but not expected.", 1, keyCount);
      }
    }
  }
};


#endif /* MANTID_CURVEFITTING_VESUVIORESOLUTIONTEST_H_ */
