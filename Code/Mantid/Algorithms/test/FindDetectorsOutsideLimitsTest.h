#ifndef FINDDETECTORSOUTSIDELIMITSTEST_H_
#define FINDDETECTORSOUTSIDELIMITSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "WorkspaceCreationHelper.hh"
#include "Poco/File.h"
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class FindDetectorsOutsideLimitsTest : public CxxTest::TestSuite
{
public:

  FindDetectorsOutsideLimitsTest()
  {
  }

  ~FindDetectorsOutsideLimitsTest()
  {}

  void testInit()
  {
    FindDetectorsOutsideLimits alg;

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

    //yVeryDead is a detector with low counts
    boost::shared_ptr<Mantid::MantidVec> yVeryDead(new Mantid::MantidVec(sizex,0.1));
    //yTooDead gives some counts at the start but has a whole region full of zeros
    double TD[sizex] = {2, 4, 5, 10, 0, 0, 0, 0, 0 , 0}; 
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
      Mantid::Geometry::Detector* det = new Mantid::Geometry::Detector("",NULL);
      det->setID(i);
      boost::shared_ptr<Mantid::Geometry::Instrument> instr = boost::dynamic_pointer_cast<Mantid::Geometry::Instrument>(work_in->getBaseInstrument());
      instr->add(det);
      instr->markAsDetector(det);
    }
    int forSpecDetMap[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    work_in->mutableSpectraMap().populate(forSpecDetMap,forSpecDetMap,20);

    FindDetectorsOutsideLimits alg;

    AnalysisDataService::Instance().add("testdead_in", work_in);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","testdead_in");
    alg.setPropertyValue("OutputWorkspace","testdead_out");
    alg.setPropertyValue("LowThreshold","1");
    alg.setPropertyValue("HighThreshold","21.01");
    alg.setPropertyValue("RangeLower", "-1");
    alg.setPropertyValue("GoodValue", liveVal);
    alg.setPropertyValue("BadValue", deadVal);
    alg.setPropertyValue("OutputFile", "FindDetectorsOutsideLimitsTestFile.txt");
    std::string filename = alg.getProperty("OutputFile");

    // Testing behavour with Range_lower or Range_upper not set
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    std::vector<int> deadDets;
    TS_ASSERT_THROWS_NOTHING( deadDets = alg.getProperty("BadSpectraNums") )
    //it will scan the whole range and so only find the very dead detectors, there are 10 of them
    TS_ASSERT_EQUALS( deadDets.size(), 11 )

    // Get back the output workspace
    MatrixWorkspace_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testdead_out")));

    for (int i=0; i< sizey; i++)
    {
      const double val = work_out->readY(i)[0];
      double valExpected = 1;
      // Spectra set up with yVeryDead fail low counts
      if ( i%2 == 0 )
      {
          valExpected = 2;
          TS_ASSERT_EQUALS( deadDets[i/2], i )
      }
      // AND yStrange fail on high
      if ( i == 19 ) valExpected = 2;
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }
    
    TS_ASSERT( Poco::File(filename).exists() )

    checkFile(filename);

    Poco::File(filename).remove();

    // Set cut off much of the range and yTooDead will stop failing on high counts
    alg.setPropertyValue("RangeUpper", "4.9");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    //retrieve the output workspace
    TS_ASSERT_THROWS_NOTHING(work_out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testdead_out")))
    //Check the dead detectors found agrees with what was setup above
    for (int i=0; i< sizey; i++)
    {
      const double val = work_out->readY(i)[0];
      double valExpected = boost::lexical_cast<double>(liveVal);
      //i%2 == 0 is the veryDead i == 19 is the yStrange
      if ( i%2==0 || i == 19) valExpected = boost::lexical_cast<double>(deadVal);
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }
    
    checkFile(filename);
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_out");
  }

private:

  void checkFile(const std::string & filename)
  {
    // Quick test number of lines within file
    std::ifstream file(filename.c_str(), std::ios_base::in );
    TS_ASSERT(file.good());

    std::string line;
    size_t line_count(0);
    while( std::getline(file, line))
    {
      line_count += 1;
    }
    file.close();
    TS_ASSERT_EQUALS(line_count, 6)

  }

};

#endif /*FINDDETECTORSOUTSIDELIMITSTEST_H_*/
