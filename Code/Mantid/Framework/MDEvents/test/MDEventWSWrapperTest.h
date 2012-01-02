#ifndef MANTID_MDEVENTS_WSWRAPPERTEST_H_
#define MANTID_MDEVENTS_WSWRAPPERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidAPI/AnalysisDataService.h"


using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class MDEventWSWrapperTest : public CxxTest::TestSuite
{
    std::auto_ptr<MDEventWSWrapper> pWSWrap;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventWSWrapperTest *createSuite() { return new MDEventWSWrapperTest(); }
  static void destroySuite( MDEventWSWrapperTest *suite ) { delete suite; }

  void test_construct()
  {
      TS_ASSERT_THROWS_NOTHING(pWSWrap = std::auto_ptr<MDEventWSWrapper>(new MDEventWSWrapper()));
  }
  void test_buildNewWS()
  {
         IMDEventWorkspace_sptr pws;
         Mantid::MDEvents::Strings targ_dim_names(5,"mdn"),targ_dim_units(5,"Momentum");
         std::vector<double> dim_min(5,-1),dim_max(5,1);

         TSM_ASSERT_THROWS("too few dimensions",pws=pWSWrap->createEmptyMDWS(0, targ_dim_names,targ_dim_units,dim_min,dim_max),std::invalid_argument);
         TSM_ASSERT_THROWS("too many dimensions",pws=pWSWrap->createEmptyMDWS(9, targ_dim_names,targ_dim_units,dim_min,dim_max),std::invalid_argument);
         TSM_ASSERT_THROWS("dimensions have not been defined ",pWSWrap->nDimensions(),std::invalid_argument);

         TSM_ASSERT_THROWS_NOTHING("should be fine",pws=pWSWrap->createEmptyMDWS(5, targ_dim_names,targ_dim_units,dim_min,dim_max));

         TSM_ASSERT_EQUALS("should have 5 dimensions",5,pWSWrap->nDimensions());

     
         TS_ASSERT_THROWS_NOTHING(pWSWrap->releaseWorkspace());

         TSM_ASSERT("should be unique",pws.unique());

  }
  void test_AddEventsData()
  {

  }

};

#endif