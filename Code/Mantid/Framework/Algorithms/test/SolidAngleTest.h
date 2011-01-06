#ifndef SOLIDANGLETEST_H_
#define SOLIDANGLETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/SolidAngle.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SolidAngleTest : public CxxTest::TestSuite
{
public:

  SolidAngleTest() : inputSpace(""), outputSpace("")
  {
    // Set up a small workspace for testing
		//Nhist = 144;
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",Nhist,11,10);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    boost::shared_ptr<Mantid::MantidVec> x(new Mantid::MantidVec(11));
    for (int i = 0; i < 11; ++i)
    {
      (*x)[i]=i*1000;
    }
    boost::shared_ptr<Mantid::MantidVec> a(new Mantid::MantidVec(10));
    boost::shared_ptr<Mantid::MantidVec> e(new Mantid::MantidVec(10));
    for (int i = 0; i < 10; ++i)
    {
      (*a)[i]=i;
      (*e)[i]=sqrt(double(i));
    }
    int forSpecDetMap[Nhist];
    for (int j = 0; j < Nhist; ++j) {
      space2D->setX(j, x);
      space2D->setData(j, a, e);
      // Just set the spectrum number to match the index
      space2D->getAxis(1)->spectraNo(j) = j+1;
      forSpecDetMap[j] = j+1;
    }

    // Register the workspace in the data service
    inputSpace = "SATestWorkspace";
    AnalysisDataService::Instance().add(inputSpace, space);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", inputSpace);
    loader.execute();

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space2D->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, Nhist );

    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    

   // Mark one detector dead to test that it leads to zero solid angle
    IDetector_sptr det143 = space2D->getDetector(143);
    ParameterMap& pmap = space2D->instrumentParameters();
    pmap.addBool(det143.get(),"masked",true);
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg.setPropertyValue("InputWorkspace",inputSpace);
    outputSpace = "outWorkspace";
    alg.setPropertyValue("OutputWorkspace",outputSpace);
  }

  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
    // Check that the output unit is correct
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF")
   
			
		const int numberOfSpectra = output2D->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
		for (int i = 0; i < numberOfSpectra-1; ++i) {
			//all of the values should fall in this range for INES
			TS_ASSERT_DELTA(output2D->readY(i)[0],0.00139,0.00001);
			
			TS_ASSERT_DELTA(output2D->readX(i)[0],0.0,0.000001);
			TS_ASSERT_DELTA(output2D->readX(i)[1],10000.0,0.000001);
			TS_ASSERT_DELTA(output2D->readE(i)[0],0.0,0.000001);
		}

		//some specific, more accurate values
		TS_ASSERT_DELTA(output2D->readY(5)[0],0.00139822,0.0000001);
		TS_ASSERT_DELTA(output2D->readY(10)[0],0.00139822,0.0000001);
		TS_ASSERT_DELTA(output2D->readY(20)[0],0.00139822,0.0000001);
		TS_ASSERT_DELTA(output2D->readY(50)[0],0.00139822,0.0000001);
    
		// Check 'dead' detector spectrum gives zero solid angle
		TS_ASSERT_EQUALS(output2D->readY(143).front(), 0)
  }

  void testExecSubset()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    alg.setPropertyValue("InputWorkspace",inputSpace);
    alg.setPropertyValue("OutputWorkspace",outputSpace);
    alg.setPropertyValue("StartWorkspaceIndex","50");
    alg.setPropertyValue("EndWorkspaceIndex","59");
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
    // Check that the output unit is correct
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF")
   
		const int numberOfSpectra = output2D->getNumberHistograms();
		TS_ASSERT_EQUALS(numberOfSpectra,10);
		for (int i = 0; i < numberOfSpectra; ++i) {
			//all of the values should fall in this range for INES
			TS_ASSERT_DELTA(output2D->readY(i)[0],0.0013,0.0001);
			
			TS_ASSERT_DELTA(output2D->readX(i)[0],0.0,0.000001);
			TS_ASSERT_DELTA(output2D->readX(i)[1],10000.0,0.000001);
			TS_ASSERT_DELTA(output2D->readE(i)[0],0.0,0.000001);
		}
    
  }

 
private:
  SolidAngle alg;
  std::string inputSpace;
  std::string outputSpace;
  enum { Nhist = 144 };
};

#endif /*SOLIDANGLETEST_H_*/
