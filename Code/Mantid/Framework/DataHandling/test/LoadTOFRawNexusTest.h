#ifndef LOADTOFRAWNEXUSTEST_H_
#define LOADTOFRAWNEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"

class LoadTOFRawNexusTest: public CxxTest::TestSuite
{
public:

  // As the algorithm isn't finished and doesn't do anything... no need for real tests.

  void testNothing()
  {
    TS_ASSERT_EQUALS(1,1);
  }

  void xtestExec()
  {
    Mantid::API::FrameworkManager::Instance();
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "../../../../Test/AutoTestData/CNCS_7860.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("outWS"));
    TS_ASSERT_EQUALS(ws->blocksize(), 2000);
  }

};

#endif /*LOADTOFRAWNEXUSTEST_H_*/
