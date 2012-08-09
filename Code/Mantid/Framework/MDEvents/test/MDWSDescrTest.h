#ifndef MDEVENTS_MDWSDESCRIPTION_TEST_H
#define MDEVENTS_MDWSDESCRIPTION_TEST_H

#include "MantidMDEvents/MDWSDescription.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDWSDescrTest : public CxxTest::TestSuite
{
    Mantid::API::MatrixWorkspace_sptr ws2D;
public:
  void testBuildFromMatrixWS2D()
  {
    MDWSDescription WSD;
    // dimensions (min-max) have not been set
    TS_ASSERT_THROWS(WSD.buildFromMatrixWS(ws2D,"|Q|","Direct"),std::invalid_argument);
    std::vector<double> dimMin(2,-1);
    std::vector<double> dimMax(2, 1);
    WSD.setMinMax(dimMin,dimMax);   
    TS_ASSERT_THROWS_NOTHING(WSD.buildFromMatrixWS(ws2D,"|Q|","Direct"));
    TS_ASSERT_EQUALS(2,WSD.nDimensions());
  }
  void testBuildFromMatrixWS4D()
  {
    MDWSDescription WSD;
    std::vector<double> dimMin(4,-10);
    std::vector<double> dimMax(4, 20);
    WSD.setMinMax(dimMin,dimMax);   
    std::vector<std::string> PropNamews(2,"Ei");
    PropNamews[1]="P";
    // no property named "P" is attached to workspace
    TS_ASSERT_THROWS(WSD.buildFromMatrixWS(ws2D,"|Q|","Direct",PropNamews),Kernel::Exception::NotFoundError);

    // H is attached
    PropNamews[1]="H";
    TS_ASSERT_THROWS_NOTHING(WSD.buildFromMatrixWS(ws2D,"|Q|","Indirect",PropNamews));
    TS_ASSERT_EQUALS(4,WSD.nDimensions());


  }

MDWSDescrTest()
{
     ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);
     // ADD time series property
     ws2D->mutableRun().addProperty("H",10.,"Gs");


}
};


#endif