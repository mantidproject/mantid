#ifndef GAUSSLEASTSQUARESFITTEST_H_
#define GAUSSLEASTSQUARESFITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GaussLeastSquaresFit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class GaussLeastSquaresFitTest : public CxxTest::TestSuite
{
public:

  //GaussLeastSquaresFitTest()
  //{
  //}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testAgainstMAR_Dataset()
  {
    if ( !alg.isInitialized() ) alg.initialize();

    // load MAR dataset to test against
    std::string inputFile = "../../../../Test/Data/MAR11060.RAW";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "MAR_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("SpectrumNumber","3");
    alg.setPropertyValue("Output y0", "-2000.0");
    alg.setPropertyValue("Output A", "8000.0");
    alg.setPropertyValue("Output xc", "10000.0");
    alg.setPropertyValue("Output w", "6000.0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 100.98,0.1);
    dummy = alg.getProperty("Output y0");
    TS_ASSERT_DELTA( dummy, -2511.4 ,0.2);
    dummy = alg.getProperty("Output A");
    TS_ASSERT_DELTA( dummy, 8620.3 ,0.2);
    dummy = alg.getProperty("Output xc");
    TS_ASSERT_DELTA( dummy, 10090.7 ,0.2);
    dummy = alg.getProperty("Output w");
    TS_ASSERT_DELTA( dummy, 6357.8 ,0.2);
  }

  void testAgainstMockData()
  {
    GaussLeastSquaresFit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "GaussMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    std::vector<double> timeChannelsVec(timechannels); // x-values
    for (int i = 0; i < 20; i++) timeChannelsVec[i] = i+1;
    std::vector<double> y(timechannels); // y-values (counts)
    y[0] =   3.56811123;
    y[1] =   3.25921675;
    y[2] =   2.69444562;
    y[3] =   3.05054488;
    y[4] =   2.86077216;
    y[5] =   2.29916480;
    y[6] =   2.57468876;
    y[7] =   3.65843827;
    y[8] =  15.31622763;
    y[9] =  56.57989073;
    y[10] = 101.20662386;
    y[11] =  76.30364797;
    y[12] =  31.54892552;
    y[13] =   8.09166673;
    y[14] =   3.20615343;
    y[15] =   2.95246554;
    y[16] =   2.75421444;
    y[17] =   3.70180447;
    y[18] =   2.77832668;
    y[19] =   2.29507565;
    std::vector<double> e(timechannels); // error values of counts
    e[0] =   1.72776328;
    e[1] =   1.74157482;
    e[2] =   1.73451042;
    e[3] =   1.73348562;
    e[4] =   1.74405622;
    e[5] =   1.72626701;
    e[6] =   1.75911386;
    e[7] =   2.11866496;
    e[8] =   4.07631054;
    e[9] =   7.65159052;
    e[10] =  10.09984173;
    e[11] =   8.95849024;
    e[12] =   5.42231173;
    e[13] =   2.64064858;
    e[14] =   1.81697576;
    e[15] =   1.72347732;
    e[16] =   1.73406310;
    e[17] =   1.73116711;
    e[18] =   1.71790285;
    e[19] =   1.72734254;
    // Populate the workspace.
    ws2D->setX(0, timeChannelsVec);
    ws2D->setData(0, y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("SpectrumNumber","1");
    alg2.setPropertyValue("Output y0", "3.0");
    alg2.setPropertyValue("Output A", "100.7");
    alg2.setPropertyValue("Output xc", "11.2");
    alg2.setPropertyValue("Output w", "1.1");

    // execute fit
    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.076185,0.0001);
    dummy = alg2.getProperty("Output y0");
    TS_ASSERT_DELTA( dummy, 2.8765 ,0.0001);
    dummy = alg2.getProperty("Output A");
    TS_ASSERT_DELTA( dummy, 97.804 ,0.001);
    dummy = alg2.getProperty("Output xc");
    TS_ASSERT_DELTA( dummy, 11.2356 ,0.0001);
    dummy = alg2.getProperty("Output w");
    TS_ASSERT_DELTA( dummy, 1.1142 ,0.0001);

  }


private:
  GaussLeastSquaresFit alg;
};

#endif /*GAUSSLEASTSQUARESFITTEST_H_*/
