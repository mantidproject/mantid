#ifndef LOADTOFRAWNEXUSTEST_H_
#define LOADTOFRAWNEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadTOFRawNeXus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"

class LoadTOFRawNeXusTest: public CxxTest::TestSuite
{
public:

  void testNothing()
  {
    TS_ASSERT_EQUALS(1,1);
  }

  void xtestExec()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadTOFRawNeXus ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "../../../../Test/AutoTestData/CNCS_7860.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("outWS"));
    TS_ASSERT_EQUALS(ws->blocksize(), 2000);
  }

};

#endif /*LOADTOFRAWNEXUSTEST_H_*/
