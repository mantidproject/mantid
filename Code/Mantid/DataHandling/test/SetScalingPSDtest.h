#ifndef SETSCALINGPSDTEST_H_
#define SETSCALINGPSDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidDataHandling/SetScalingPSD.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument/ParObjComponent.h"
#include <stdexcept>



using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class SetScalingPSDTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "SetScalingPSD" )
  }

  void testInit()
  {
    alg.initialize();
    TS_ASSERT( alg.isInitialized() )
  }

  void testLoadMer()
  {
    //std::string inputFile = "../../../../Test/AutoTestData/HET15869.raw";
    std::string inputFile = "../../../../Test/Data/MER02257.raw";
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "testWS";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.setPropertyValue("SpectrumMin","1");
    loader.setPropertyValue("SpectrumMax","100");

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    //
    // get workspace
    //
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)) );
    //Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
   }

  void testInitProperties()
  {
    // for testing we only use a small part of the full scaling file as it takes too long
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ScalingFileName", "../../../../Test/Data/merlin_detector.sca"));
    //alg.setPropertyValue("ScalingFileName", "../../../../Test/Data/merlin_detector_partial.sca");
    alg.setPropertyValue("Workspace", "testWS");
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = alg.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare("testWS"));
  }

  void testExecute()
  {
      // before SetScalingPSD is executated

      IInstrument_sptr inst0 = output->getInstrument();
      // get pointer to the first detector in bank 2
      boost::shared_ptr<IDetector> det0 = inst0->getDetector(2110001);

      int id0=det0->getID();
      TS_ASSERT_EQUALS(2110001,id0);
      V3D pos0 = det0->getPos();
      V3D expectedPos0 = V3D(-0.99999,-1.46357,2.29129);
      TS_ASSERT_DELTA((pos0-expectedPos0).norm(),0.0,1e-5)

      try 
      {
          TS_ASSERT_EQUALS(alg.execute(),true);
      }
      catch(std::runtime_error e)
      {
          TS_FAIL(e.what());
      }


      // after SetScalingPSD is executated

      IInstrument_sptr inst = output->getInstrument();
      // get pointer to the first detector in bank 2
      boost::shared_ptr<IDetector> det = inst->getDetector(2110001);

      int id=det->getID();
      TS_ASSERT_EQUALS(2110001,id);
      V3D pos = det->getPos();
      V3D expectedPos = V3D(-1.0,-1.51453,2.29129);
      TS_ASSERT_DELTA((pos-expectedPos).norm(),0.0,1e-5)
      
      // check that points lie on correct side of the scaled object
      boost::shared_ptr<ParObjComponent> pdet= boost::dynamic_pointer_cast<ParObjComponent>(det);
      //double yHW=0.00003;
      //TS_ASSERT( pdet->isValid(expectedPos) )
      //TS_ASSERT( pdet->isValid(expectedPos+V3D(0,0.99*yHW,0)) )
      //TS_ASSERT( pdet->isValid(expectedPos-V3D(0,0.99*yHW,0)) )
      //TS_ASSERT( pdet->isValid(expectedPos+V3D(0,1.05*yHW,0)) )
      //TS_ASSERT( pdet->isValid(expectedPos-V3D(0,1.05*yHW,0)) )
      //TS_ASSERT( ! pdet->isValid(expectedPos+V3D(0,1.06*yHW,0)) )
      //TS_ASSERT( ! pdet->isValid(expectedPos-V3D(0,1.06*yHW,0)) )
      // check track behaves correctly
      //Track track(V3D(0,0,0),expectedPos/expectedPos.norm());
      //TS_ASSERT_EQUALS(pdet->interceptSurface(track),1)
      //Track::LType::const_iterator it=track.begin();
      //TS_ASSERT(it != track.end()) 
      // lenght of track is not correct under scaling at present
      //TS_ASSERT_DELTA( it->Dist,2.92297968+0.5e-5,2e-6 )
      //TS_ASSERT_DELTA( it->Length, 1e-5, 1e-7 )
      //std::cout << "sa new=" << sa << " sa ori=" << sa0 << "  ratio=" << sa/sa0
      //    << " distance ratio=" << pos.norm()/pos0.norm() << std::endl ;
  }

private:
  SetScalingPSD alg;
  Mantid::DataHandling::LoadRaw2 loader;
  std::string outputSpace;
  MatrixWorkspace_sptr output;

};

#endif /*SETSCALINGPSDTEST_H_*/
