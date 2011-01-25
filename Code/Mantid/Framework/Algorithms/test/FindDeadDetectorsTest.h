#ifndef FINDDEADDETECTORSTEST_H_
#define FINDDEADDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class FindDeadDetectorsTest : public CxxTest::TestSuite
{
public:

  FindDeadDetectorsTest()
  {
  }

  ~FindDeadDetectorsTest()
  {}

  void testInit()
  {
    FindDeadDetectors alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testExec()
  {
    const std::string liveVal = "1", deadVal = "2";
    const int sizex = 10, sizey = 20;
    // Register the workspace in the data service and initialise it with abitary data
    Workspace2D_sptr work_in =
    //the x values look like this -1, 2, 5, 8, 11, 14, 17, 20, 23, 26
      WorkspaceCreationHelper::Create2DWorkspaceBinned(sizey, sizex, -1, 3.0);

    //yVeryDead is a detector that never responds and produces no counts
    boost::shared_ptr<Mantid::MantidVec> yVeryDead(new Mantid::MantidVec(sizex,0));
    //yTooDead gives some counts at the start but has a whole region full of zeros
    double TD[sizex] = {2, 4, 5, 1, 0, 0, 0, 0, 0 , 0}; 
    boost::shared_ptr<Mantid::MantidVec> yTooDead(new Mantid::MantidVec(TD, TD+10));
    //yStrange dies after giving some counts but then comes back
    double S[sizex] = {0.2, 4, 50, 0.001, 0, 0, 0, 0, 1 , 0}; 
    boost::shared_ptr<Mantid::MantidVec> yStrange(new Mantid::MantidVec(S, S+10));
    for (int i=0; i< sizey; i++)
    {
      if (i%3 == 0)
      {//the last column is set arbitrarily to have the same values as the second because the errors shouldn't make any difference
        work_in->setData(i, yTooDead, yTooDead);
      }
      if (i%2 == 0)
      {
        work_in->setData(i, yVeryDead, yVeryDead);
      }
      if (i == 19)
      {
        work_in->setData(i, yStrange, yTooDead);
      }
      work_in->getAxis(1)->spectraNo(i) = i;
      Mantid::Geometry::Detector* det = new Mantid::Geometry::Detector("",0,NULL);
      boost::shared_ptr<Mantid::Geometry::Instrument> instr = boost::dynamic_pointer_cast<Mantid::Geometry::Instrument>(work_in->getBaseInstrument());
      instr->add(det);
      instr->markAsDetector(det);
    }
    int forSpecDetMap[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    work_in->mutableSpectraMap().populate(forSpecDetMap,forSpecDetMap,20);

    FindDeadDetectors alg;

    AnalysisDataService::Instance().add("testdead_in", work_in);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","testdead_in");
    alg.setPropertyValue("OutputWorkspace","testdead_out");
    alg.setPropertyValue("DeadThreshold","0");
    alg.setPropertyValue("LiveValue", liveVal);
    alg.setPropertyValue("DeadValue", deadVal);
    std::string filename = "testFile.txt";
    alg.setPropertyValue("OutputFile",filename);

    // Testing behavour with Range_lower or Range_upper not set
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    std::vector<int> deadDets;
    TS_ASSERT_THROWS_NOTHING( deadDets = alg.getProperty("FoundDead") )
    //it will scan the whole range and so only find the very dead detectors, there are 10 of them
    TS_ASSERT_EQUALS( deadDets.size(), 10 )

    // Get back the output workspace
    MatrixWorkspace_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testdead_out")));

    for (int i=0; i< sizey; i++)
    {
      const double val = work_out->readY(i)[0];
      double valExpected = 1;
      if (i%2==0)
      {
          valExpected = 2;
          TS_ASSERT_EQUALS( deadDets[i/2], i )
      }
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }

    std::fstream outFile(filename.c_str());
    TS_ASSERT( outFile )
    outFile.close();
    Poco::File(filename).remove();

    // Set Range_lower to later in the histogram when the yTooDead detectors stop working
    alg.setPropertyValue("RangeLower", "11.0");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    //retrieve the output workspace
    TS_ASSERT_THROWS_NOTHING(
      work_out = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("testdead_out")
      ))
    //Check the dead detectors found agrees with what was setup above
    for (int i=0; i< sizey; i++)
    {
      const double val = work_out->readY(i)[0];
      double valExpected = boost::lexical_cast<double>(liveVal);
      //i%2 == 0 is the veryDead i%3 == 0 is the TooDead
      if ( i%2==0 || i%3 == 0) valExpected = boost::lexical_cast<double>(deadVal);
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }

    // Set Range_upper to before the end which will pickup the strange
    alg.setPropertyValue("RangeUpper", "20");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    //retrieve the output workspace
    TS_ASSERT_THROWS_NOTHING(
      work_out = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("testdead_out")
      ))
    //Check the dead detectors found agrees with what was setup above
    for (int i=0; i< sizey; i++)
    {
      const double val = work_out->readY(i)[0];
      double valExpected = boost::lexical_cast<double>(liveVal);
      //i%2 == 0 is the veryDead i%3 == 0 is the TooDead i == 19 is the strange
      if ( i%2==0 || i%3 == 0 || i == 19) valExpected = boost::lexical_cast<double>(deadVal);
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_out");
  }
};

#endif /*FINDDEADDETECTORSTEST_H_*/
