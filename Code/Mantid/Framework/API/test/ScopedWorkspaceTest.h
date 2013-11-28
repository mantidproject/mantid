#ifndef MANTID_API_SCOPEDWORKSPACETEST_H_
#define MANTID_API_SCOPEDWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;

class ScopedWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScopedWorkspaceTest *createSuite() { return new ScopedWorkspaceTest(); }
  static void destroySuite( ScopedWorkspaceTest *suite ) { delete suite; }

  ScopedWorkspaceTest() :
    ads( AnalysisDataService::Instance() )
  {
    ads.clear();
  }

  ~ScopedWorkspaceTest()
  {
    ads.clear();
  }

  void test_emptyConstructor()
  {
    ScopedWorkspace test;
    // Should have name created
    TS_ASSERT( ! test.name().empty() );
    // However, nothing should be added under that name yet
    TS_ASSERT( ! ads.doesExist( test.name() ) );
  }

private:
  AnalysisDataServiceImpl& ads;
};


#endif /* MANTID_API_SCOPEDWORKSPACETEST_H_ */
