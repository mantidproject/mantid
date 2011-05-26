#ifndef FIND_DETECTORSPAR_H_
#define FIND_DETECTORSPAR_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

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
    TS_ASSERT_EQUALS( findPar->category(), "DataHandling\\Detectors" );
  }

  void testInit(){

    TS_ASSERT_THROWS_NOTHING( findPar->initialize() );
    TS_ASSERT( findPar->isInitialized() );

    TSM_ASSERT_EQUALS("should be six propeties here",6,(size_t)(findPar->getProperties().size()));
  }

 //void t__tSimpleExec(){
 //     inputWS = buildUngroupedWS("FindDetParTestWS");
 //
 //     TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));

 //     TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
 //     TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );
 // 
 //}
 //void t__tSimpleResults(){
 //    TS_ASSERT_EQUALS(std::string("0,0,0"),                     findPar->getPropertyValue("azimuthal"));
 //    TS_ASSERT_EQUALS(std::string("170.565,169.565,168.565"),   findPar->getPropertyValue("polar"));
 //    TS_ASSERT_EQUALS(std::string("0.396157,0.394998,0.393718"),findPar->getPropertyValue("azimuthal_width"));
 //    TS_ASSERT_EQUALS(std::string("2.86236,2.86236,2.86236"),   findPar->getPropertyValue("polar_width"));
 //    TS_ASSERT_EQUALS(std::string("1,1,1"),                     findPar->getPropertyValue("secondary_flightpath"));
 //}
 //void t__tSingleRingExec(){
 //    inputWS =buildRingGroupedWS("FindDetRingParTestWS");

 //    TS_ASSERT_THROWS_NOTHING(findPar->setPropertyValue("InputWorkspace", inputWS->getName()));

 //    TSM_ASSERT_THROWS_NOTHING("Calculating workspace parameters should not throw", findPar->execute() );
 //    TSM_ASSERT("parameters calculations should complete successfully", findPar->isExecuted() );

 //}
 // void t__tSingleRingResults(){
 //    TS_ASSERT_EQUALS(std::string("27.8095"),findPar->getPropertyValue("azimuthal"));
 //    TS_ASSERT_EQUALS(std::string("0"),      findPar->getPropertyValue("polar"));
 //    TS_ASSERT_EQUALS(std::string("35.8642"),findPar->getPropertyValue("azimuthal_width"));
 //    TS_ASSERT_EQUALS(std::string("6.28319"),findPar->getPropertyValue("polar_width"));
 //    TS_ASSERT_EQUALS(std::string("6.82302"),findPar->getPropertyValue("secondary_flightpath"));
 //   

 //}

 //*******************************************************
 FindDetectorsParTest()
 {// the functioning of FindDetectorsParTest is affected by a function call in the FrameworkManager's constructor, creating the algorithm in this way ensures that function is executed
    
    findPar =  FrameworkManager::Instance().createAlgorithm("FindDetectorsPar");
  }
 ~FindDetectorsParTest(){
      FrameworkManager::Instance().clearAlgorithms();
      FrameworkManager::Instance().deleteWorkspace(inputWS->getName());
 }
private:
    IAlgorithm* findPar;
    MatrixWorkspace_sptr inputWS;
    std::vector<Geometry::IDetector_sptr>  partDetectors;

    //
    MatrixWorkspace_sptr  buildUngroupedWS(const std::string &WS_Name)
    {
        const size_t NHIST=3;

        inputWS  = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,10,1.0);

        specid_t forSpecDetMap[NHIST];
        for (size_t j = 0; j < NHIST; ++j)
        {
            // Just set the spectrum number to match the index
            inputWS->getAxis(1)->spectraNo(j) = specid_t(j+1);
            forSpecDetMap[j] = specid_t(j+1);
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

       //  inputWS->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, NHIST);
         return inputWS;

    }
    MatrixWorkspace_sptr  buildRingGroupedWS(const std::string &WS_Name)
    {
       if(inputWS.get()){
           AnalysisDataService::Instance().remove(inputWS->getName());
       }

       boost::shared_ptr<Geometry::DetectorGroup> pDet(ComponentCreationHelper::createRingOfCylindricalDetectors(2,4,4));
       const size_t NDET=pDet->nDets();

       inputWS  = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,10,1.0);

       boost::shared_ptr<Geometry::Instrument> spInst(new Geometry::Instrument("basic_ring"));
       Geometry::ObjComponent *source = new Geometry::ObjComponent("source");
       source->setPos(0.0,0.0,-10.0);
       spInst->markAsSource(source);

       Geometry::ObjComponent *sample = new Geometry::ObjComponent("sample");
       sample->setPos(0.0,0.0,-2);
       spInst->markAsSamplePos(sample);

       // get pointers to the detectors, contributed into group;
       partDetectors = pDet->getDetectors();

       std::vector<specid_t>forSpecDetMap(NDET);
       inputWS->getAxis(1)->spectraNo(0) = 1;

       for(size_t i=0;i<NDET;i++){
            spInst->markAsDetector(partDetectors[i].get());
        // Just set the spectrum number to match the index
            forSpecDetMap[i] = 1;

       }
      inputWS->setInstrument(spInst);
             
  
      // underlying detectors have different id-s but the group has the ID of the first one'
      std::vector<detid_t> detIDDetails = pDet->getDetectorIDs();

     // inputWS->mutableSpectraMap().populate(&forSpecDetMap[0],&detIDDetails[0], NDET);

      AnalysisDataService::Instance().add(WS_Name,inputWS);
      return inputWS;

    }
};
#endif
