#ifndef MDEVENTS_MDWSDESCRIPTION_TEST_H
#define MDEVENTS_MDWSDESCRIPTION_TEST_H

#include "MantidMDEvents/MDWSDescription.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::MDEvents;

class MDWSDescrTest : public CxxTest::TestSuite
{
    Mantid::API::MatrixWorkspace_sptr ws2D;
public:
  void testBuildFromMatrixWS()
  {
    MDWSDescription WSD;
    TS_ASSERT_THROWS_NOTHING(WSD.buildFromMatrixWS(ws2D,"|Q|","Direct"));

  }

MDWSDescrTest()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);


}
};


#endif