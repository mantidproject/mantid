#ifndef FIND_DETECTORSPAR_H_
#define FIND_DETECTORSPAR_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class FindDetectorsParTest : public CxxTest::TestSuite
{
public:
 static FindDetectorsParTest *createSuite() { return new FindDetectorsParTest(); }
 static void destroySuite(FindDetectorsParTest *suite) { delete suite; }
 //*******************************************************
  void testName(){
    TS_ASSERT_EQUALS( findPar->name(), "FindDetectorsPar" );
  }

  void testVersion(){
    TS_ASSERT_EQUALS( findPar->version(), 1 );
  }

  void testCategory(){
    TS_ASSERT_EQUALS( findPar->category(), "DataHandling" );
  }

  void testInit(){

    TS_ASSERT_THROWS_NOTHING( findPar->initialize() );
    TS_ASSERT( findPar->isInitialized() );

    TSM_ASSERT_EQUALS("should be six propeties here",6,(size_t)(findPar->getProperties().size()));
  }

 void testExec(){
 
	  TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));

	  TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
      TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );
  
 }
 void testResults(){
	 TSM_ASSERT_EQUALS("Azimutal values should be exactly specified",std::string("0,0,0"),findPar->getPropertyValue("azimuthal"));
	 TSM_ASSERT_EQUALS("Polar values should be exactly specified",std::string("170.565,169.565,168.565"),findPar->getPropertyValue("polar"));
	 TSM_ASSERT_EQUALS("Azimutal width values should be exactly specified",std::string("0.396108,0.39495,0.393671"),findPar->getPropertyValue("azimuthal_width"));
	 TSM_ASSERT_EQUALS("Polar width values should be exactly specified",std::string("2.86241,2.86241,2.86241"),findPar->getPropertyValue("polar_width"));
	 TSM_ASSERT_EQUALS("Flightpath values should be exactly  specified",std::string("1,1,1"),findPar->getPropertyValue("secondary_flightpath"));
 }

 //*******************************************************
 FindDetectorsParTest()
 {// the functioning of FindDetectorsParTest is affected by a function call in the FrameworkManager's constructor, creating the algorithm in this way ensures that function is executed
	const size_t NHIST=3;
	std::string WS_Name("FindDetParTestWS");
    findPar =  FrameworkManager::Instance().createAlgorithm("FindDetectorsPar");
	inputWS  = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);

    int forSpecDetMap[NHIST];
    for (size_t j = 0; j < NHIST; ++j)
    {
      // Just set the spectrum number to match the index
      inputWS->getAxis(1)->spectraNo(j) = j+1;
      forSpecDetMap[j] = j+1;
    }

    AnalysisDataService::Instance().add(WS_Name,inputWS);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", WS_Name);
    loader.execute();

    inputWS->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, NHIST);

 }
 ~FindDetectorsParTest(){
	  FrameworkManager::Instance().clearAlgorithms();
	  FrameworkManager::Instance().deleteWorkspace(inputWS->getName());
 }
private:
	IAlgorithm* findPar;
    MatrixWorkspace_sptr inputWS;
};
#endif
