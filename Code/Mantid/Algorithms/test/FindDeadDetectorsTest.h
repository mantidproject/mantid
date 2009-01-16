#ifndef FINDDEADDETECTORSTEST_H_
#define FINDDEADDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "WorkspaceCreationHelper.hh"
#include <fstream>

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
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    Workspace2D_sptr work_in = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey,1);
    int forSpecDetMap[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    //set some dead detectors
    std::vector<double> yDead(sizex,0);
    for (int i=0; i< sizey; i++)
    {
      if (i%2==0)
      {
        work_in->setData(i,yDead,yDead);
      }
      work_in->getAxis(1)->spectraNo(i) = i;
      Mantid::Geometry::Detector* det = new Mantid::Geometry::Detector("",NULL);
      det->setID(i);
      boost::shared_ptr<Mantid::API::Instrument> instr = boost::dynamic_pointer_cast<Mantid::API::Instrument>(work_in->getInstrument());
      instr->add(det);
      instr->markAsDetector(det);
    }
    work_in->getSpectraMap()->populate(forSpecDetMap,forSpecDetMap,20);

    FindDeadDetectors alg;

    AnalysisDataService::Instance().add("testdead_in", work_in);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","testdead_in");
    alg.setPropertyValue("OutputWorkspace","testdead_out");
    alg.setPropertyValue("DeadThreshold","0");
    alg.setPropertyValue("LiveValue","1");
    alg.setPropertyValue("DeadValue","2");
    std::string filename = "testFile.txt";
    alg.setPropertyValue("OutputFile",filename);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the output property
    std::vector<int> deadDets;
    TS_ASSERT_THROWS_NOTHING( deadDets = alg.getProperty("FoundDead") )
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
    remove(filename.c_str());

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_out");

  }

};

#endif /*FINDDEADDETECTORSTEST_H_*/
