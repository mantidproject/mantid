#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/IntegratePeaksHybrid.h"

#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"


using namespace Mantid::Crystal;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

//=====================================================================================
// Functional Tests
//=====================================================================================
class IntegratePeaksHybridTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksHybridTest *createSuite() { return new IntegratePeaksHybridTest(); }
  static void destroySuite( IntegratePeaksHybridTest *suite ) { delete suite; }


  void test_Init()
  {
    IntegratePeaksHybrid alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_peaks_workspace_mandatory()
  {
    IMDHistoWorkspace_sptr mdws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1,1);

    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", mdws);
    alg.setProperty("BackgroundOuterRadius", 1.0);
    alg.setPropertyValue("OutputWorkspaces", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    TSM_ASSERT_THROWS("PeaksWorkspace required", alg.execute(), std::runtime_error&);
  }

  void test_input_md_workspace_mandatory()
  {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace();

    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setPropertyValue("OutputWorkspaces", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    alg.setProperty("BackgroundOuterRadius", 1.0);
    TSM_ASSERT_THROWS("InputWorkspace required", alg.execute(), std::runtime_error&);
  }


};


#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_ */
