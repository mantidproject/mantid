#ifndef PLUSTEST_H_
#define PLUSTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Plus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAlgorithms/PolynomialCorrection.h"
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class WorkspaceGroupTest : public CxxTest::TestSuite
{
private:
	void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1)
  {
    //default to a horizontal loop orientation
    checkData(work_in1,work_in2,work_out1,0);
  }

  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, int loopOrientation)
  {
    int ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (int i = 0; i < work_out1->size(); i++)
    {
      int ws2Index = i;
    
      if (ws2LoopCount > 1)
      {
        if (loopOrientation == 0)
        {
          ws2Index = i%ws2LoopCount;
        }
        else
        {
          ws2Index = i/ws2LoopCount;
        }
      }
      checkDataItem(work_in1,work_in2,work_out1,i,ws2Index);
    }
  }

  void checkDataItem (MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, int i, int ws2Index)
  {
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->dataY(ws2Index/work_in1->blocksize())[ws2Index%work_in2->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      
      TS_ASSERT_DELTA(sig1 + sig2, sig3, 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double err3(sqrt((err1*err1) + (err2*err2)));     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }
public:

  void testExecwithOneGroupandOne2DWorkspace()
  {
	  //std::string s;
   //std::getline(std::cin,s);
     int sizex = 10,sizey=20;
    // Register the workspace in the data service
   	MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
	MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in4= WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

	WorkspaceGroup_sptr wsSptr= WorkspaceGroup_sptr(new WorkspaceGroup);
	if(wsSptr)
	{
		AnalysisDataService::Instance().add("test_in", wsSptr);
		wsSptr->add("test_in");
		AnalysisDataService::Instance().add("test_in_1", work_in1);
		wsSptr->add("test_in_1");
		AnalysisDataService::Instance().add("test_in_2", work_in2);
		wsSptr->add("test_in_2");
		AnalysisDataService::Instance().add("test_in_3", work_in3);
		wsSptr->add("test_in_3");
		AnalysisDataService::Instance().add("test_in_4", work_in4);
		wsSptr->add("test_in_4");
	}
    WorkspaceGroup_sptr work_in;
	work_in=boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("test_in"));
	TS_ASSERT_EQUALS(work_in,wsSptr);
	if(work_in) 
	{
		std::vector<std::string> GroupNames=work_in->getNames();
		int nSize=GroupNames.size();
		TS_ASSERT_EQUALS(nSize,5);
	}
	Plus alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1","test_in");
    alg.setPropertyValue("InputWorkspace_2","test_in_1");    
    alg.setPropertyValue("OutputWorkspace","test_out");
    alg.execute();
	TS_ASSERT( alg.isExecuted() );

    WorkspaceGroup_sptr work_out;
	TS_ASSERT_THROWS_NOTHING(work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("test_out")));
	MatrixWorkspace_sptr work_out1 ;
	TS_ASSERT_THROWS_NOTHING(work_out1= boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out_1")));
	MatrixWorkspace_sptr work_out2;
	TS_ASSERT_THROWS_NOTHING(work_out2= boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out_2")));
	MatrixWorkspace_sptr work_out3;
	TS_ASSERT_THROWS_NOTHING(work_out3= boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out_3")));
	MatrixWorkspace_sptr work_out4;
	TS_ASSERT_THROWS_NOTHING(work_out4= boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out_4")));

    
    checkData(work_in1, work_in1, work_out1);
	checkData(work_in2, work_in1, work_out2);
	checkData(work_in3, work_in1, work_out3);
	checkData(work_in4, work_in1, work_out4);

	work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("test_out"));
	work_out->removeAll();

    AnalysisDataService::Instance().remove("test_in");
    AnalysisDataService::Instance().remove("test_in_1");
    AnalysisDataService::Instance().remove("test_in_2");
	AnalysisDataService::Instance().remove("test_in_3");
    AnalysisDataService::Instance().remove("test_in_4");
    AnalysisDataService::Instance().remove("test_out");
	AnalysisDataService::Instance().remove("test_out_1");
    AnalysisDataService::Instance().remove("test_out_2");
	AnalysisDataService::Instance().remove("test_out_3");
    AnalysisDataService::Instance().remove("test_out_4");

  }
  
  void testExecOnlyOneGroupInput()
  {
	int sizex = 10,sizey=20;
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey,1);
	Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey,1);
   int forSpecDetMap[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    //set some dead detectors
    boost::shared_ptr<Mantid::MantidVec> yDead(new Mantid::MantidVec(sizex,0));
    for (int i=0; i< sizey; i++)
    {
      if (i%2==0)
      {
        work_in1->setData(i,yDead,yDead);
      }
      work_in1->getAxis(1)->spectraNo(i) = i;
      Mantid::Geometry::Detector* det = new Mantid::Geometry::Detector("",NULL);
      det->setID(i);
      boost::shared_ptr<Mantid::API::Instrument> instr = boost::dynamic_pointer_cast<Mantid::API::Instrument>(work_in1->getInstrument());
      instr->add(det);
      instr->markAsDetector(det);
    }
    work_in1->mutableSpectraMap().populate(forSpecDetMap,forSpecDetMap,20);

	for (int i=0; i< sizey; i++)
    {
      if (i%2==0)
      {
        work_in2->setData(i,yDead,yDead);
      }
      work_in2->getAxis(1)->spectraNo(i) = i;
      Mantid::Geometry::Detector* det = new Mantid::Geometry::Detector("",NULL);
      det->setID(i);
      boost::shared_ptr<Mantid::API::Instrument> instr = boost::dynamic_pointer_cast<Mantid::API::Instrument>(work_in2->getInstrument());
      instr->add(det);
      instr->markAsDetector(det);
    }
    work_in2->mutableSpectraMap().populate(forSpecDetMap,forSpecDetMap,20);
   
	WorkspaceGroup_sptr wsSptr= WorkspaceGroup_sptr(new WorkspaceGroup);
	AnalysisDataService::Instance().add("testdead_in", wsSptr);
	wsSptr->add("testdead_in");
	AnalysisDataService::Instance().add("testdead_in_1", work_in1);
	wsSptr->add("testdead_in_1");
	AnalysisDataService::Instance().add("testdead_in_2", work_in2);
	wsSptr->add("testdead_in_2");

	WorkspaceGroup_sptr work_in;
	work_in=boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("testdead_in"));
	TS_ASSERT_EQUALS(work_in,wsSptr);
	
	 FindDeadDetectors alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","testdead_in");
    alg.setPropertyValue("OutputWorkspace","testdead_out");
    alg.setPropertyValue("DeadThreshold","0");
    alg.setPropertyValue("LiveValue","1");
    alg.setPropertyValue("DeadValue","2");
	std::string filename = "testFile.txt";
    alg.setPropertyValue("OutputFile",filename);
	alg.execute();

	TS_ASSERT_THROWS_NOTHING(alg.execute());

	TS_ASSERT( alg.isExecuted() );
	// Get back the output workspace
	WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("testdead_out")));
   
  	MatrixWorkspace_sptr work_out1;
	TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testdead_out_1")));

	MatrixWorkspace_sptr work_out2 ;
	TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testdead_out_2")));

    // Get back the output property
    std::vector<int> deadDets;
    TS_ASSERT_THROWS_NOTHING( deadDets = alg.getProperty("FoundDead") )
    TS_ASSERT_EQUALS( deadDets.size(),0 )
    for (int i=0; i< sizey; i++)
    {
      const double val = work_out1->readY(i)[0];
      double valExpected = 1;
      if (i%2==0)
      {
          valExpected = 2;
		  if(!deadDets.empty())
			TS_ASSERT_EQUALS( deadDets[i/2], i )
      }
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }

	std::fstream outFile(filename.c_str());
    TS_ASSERT( outFile )
    outFile.close();
    remove(filename.c_str());

   work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("testdead_out"));
   work_out->removeAll();

   AnalysisDataService::Instance().remove("testdead_in");
   AnalysisDataService::Instance().remove("testdead_in_1");
   AnalysisDataService::Instance().remove("testdead_in_2");
   AnalysisDataService::Instance().remove("testdead_out");
   AnalysisDataService::Instance().remove("testdead_out_1");
   AnalysisDataService::Instance().remove("testdead_out_2");


  }
   void testExecGroupwithNochildWorkspaces()
   {
	   WorkspaceGroup_sptr wsSptr= WorkspaceGroup_sptr(new WorkspaceGroup);
	   AnalysisDataService::Instance().add("InputWS", wsSptr);
	   wsSptr->add("InputWS");
  	   Mantid::Algorithms::PolynomialCorrection poly3;
	   poly3.initialize();
	   TS_ASSERT_THROWS_NOTHING( poly3.setPropertyValue("InputWorkspace","InputWS") )
	   TS_ASSERT_THROWS_NOTHING( poly3.setPropertyValue("OutputWorkspace","WSCor") )
	   TS_ASSERT_THROWS_NOTHING( poly3.setPropertyValue("Coefficients","3.0,2.0,1.0") )
	   TS_ASSERT(! poly3.execute() )
	   //TS_ASSERT( poly3.isExecuted() )

	  // MatrixWorkspace_sptr result;
	   //TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("WSCor")) )


   }


};

#endif /*PLUSTEST_H_*/
