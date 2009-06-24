#ifndef FINDPROBLEMDETECTORSTEST_H_
#define FINDPROBLEMDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/FindProblemDetectors.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class FindProblemDetectorsTest : public CxxTest::TestSuite
{
public:

  FindProblemDetectorsTest()
  {
    using namespace Mantid;
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",Nhist,11,10);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    boost::shared_ptr<MantidVec> x(new MantidVec(11));
    for (int i = 0; i < 11; ++i)
    {
      (*x)[i]=i*1000;
    }
    // 21 random numbers that will be copied into the workspace spectra
    const short ySize = 21;
    double yArray[ySize] =
      {0.2,4,50,0.001,0,0,0,1,0,15,4,0,0.001,2e-10,0,8,0,1e-4,1,7,11};

    //the error values aren't used and aren't tested so we'll use some basic data
    boost::shared_ptr<MantidVec> errors( new MantidVec( ySize, 1) );
    boost::shared_ptr<MantidVec> spectrum;

    for (int j = 0; j < Nhist; ++j)
    {
      space2D->setX(j, x);
      spectrum.reset( new MantidVec );
      //the spectrums are multiples of the random numbers above
      for ( int l = 0; l < ySize; ++l )
      {
        spectrum->push_back( j*yArray[l] );
      }
      space2D->setData( j, spectrum, errors );
      // Just set the spectrum number to match the index
      space2D->getAxis(1)->spectraNo(j) = j+1;
    }
    
    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    int forSpecDetMap[Nhist];
    for ( int j = 0; j < Nhist; j++ ) forSpecDetMap[j] = j+1;
    space2D->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, Nhist );
    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("FindProbDetectsTestInput", space);
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg.setPropertyValue("WhiteBeamWorkspace","FindProbDetectsTestInput");
    alg.setPropertyValue("OutputWorkspace","FindProbDetectsTestOutput");
  }

  void testWithoutAngles()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("FindProbDetectsTestOutput"));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve("FindProbDetectsTestInput"));
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("FindProbDetectsTestOutput"));
    MatrixWorkspace_sptr outputMat = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if ( !outputMat ) throw std::logic_error("outputMat is null or not a Matrix workspace, aborting some asserts");
    TS_ASSERT_EQUALS( outputMat->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT_EQUALS( outputMat->YUnit(), " per microsecond" )

    //were all the spectra output?
		const int numberOfSpectra = outputMat->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
  }

  void testWithSolidAngles()
  {
    // Get input workspace back to add detector geometry info to
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve("FindProbDetectsTestInput"));
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
   
    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "../../../../Test/Instrument/INS_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", "FindProbDetectsTestInput");
    loader.execute();

    
    // Mark one detector dead to test that it leads to zero solid angle
    Detector* det143 = dynamic_cast<Detector*>(input2D->getDetector(143).get());
    boost::shared_ptr<ParameterMap> pmap = input2D->instrumentParameters();
    pmap->addBool(det143,"masked",true);

    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
  }

  void testExecSubset()
  {/*
    if ( !alg.isInitialized() ) alg.initialize();
    alg.setPropertyValue("InputWorkspace",inputSpace);
    alg.setPropertyValue("OutputWorkspace",outputSpace);
    alg.setPropertyValue("StartSpectrum","50");
    alg.setPropertyValue("EndSpectrum","59");
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
    */
  }

 
private:
  FindProblemDetectors alg;

  enum { Nhist = 144 };
};

#endif /*FINDPROBLEMDETECTORSTEST_H_*/
