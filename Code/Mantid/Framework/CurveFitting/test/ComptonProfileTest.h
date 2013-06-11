#ifndef MANTID_CURVEFITTING_COMPTONPROFILETEST_H_
#define MANTID_CURVEFITTING_COMPTONPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ComptonProfile.h"

using Mantid::CurveFitting::ComptonProfile;

class ComptonProfileTest : public CxxTest::TestSuite
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComptonProfileTest *createSuite() { return new ComptonProfileTest(); }
  static void destroySuite( ComptonProfileTest *suite ) { delete suite; }

  void test_initialized_object_has_no_parameters()
  {
    auto profile = createFunction();
    TS_ASSERT_EQUALS(0, profile->nParams());
  }

  void test_initialized_object_has_expected_attributes()
  {
    auto profile = createFunction();
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
  struct FakeComptonProfile : ComptonProfile
  {
    std::string name() const { return "FakeComptonProfile"; }
    size_t numConstraintMatrixColumns() const { return 0; }
    void fillConstraintMatrix(Mantid::Kernel::DblMatrix &,const size_t,const std::vector<double>&) const {}

    void massProfile(double *,const size_t) const {}
  };

  Mantid::API::IFunction_sptr createFunction()
  {
    auto profile = boost::make_shared<FakeComptonProfile>();
    profile->initialize();
    return profile;
  }

};


#endif /* MANTID_CURVEFITTING_COMPTONPROFILETEST_H_ */
