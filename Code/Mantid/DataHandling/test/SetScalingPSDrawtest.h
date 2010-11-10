#ifndef SETSCALINGPSDTESTRAW_H
#define SETSCALINGPSDTESTRAW_H

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidDataHandling/SetScalingPSD.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include <stdexcept>


using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class SetScalingPSDRawTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "SetScalingPSD" );
  }

  void testInit()
  {
    alg.initialize();
    TS_ASSERT( alg.isInitialized() );
  }

  void testLoadMer()
  {
    std::string inputFile = "../../../../Test/AutoTestData/MER02257.raw";
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
    // get scaling from raw file
    alg.setPropertyValue("ScalingFilename", "../../../../Test/AutoTestData/MER02257.raw");
    alg.setPropertyValue("Workspace", "testWS");
       alg.setPropertyValue("ScalingOption", "2");
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = alg.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare("testWS"));
  }

  void testExecute()
  {
      IInstrument_sptr inst0 = output->getInstrument();
      // get pointer to the first detector in bank 2
      boost::shared_ptr<IDetector> det0 = inst0->getDetector(2110001);

      int id0=det0->getID();
      TS_ASSERT_EQUALS(2110001,id0);
      V3D pos0 = det0->getPos();
      V3D expectedPos0 = V3D(-0.99999,-1.46357,2.29129);
      TS_ASSERT_DELTA((pos0-expectedPos0).norm(),0.0,1e-5)
      //double sa0=det0->solidAngle(V3D(0,0,0));
      //TS_ASSERT_DELTA(sa0,7.4785e-6,1e-10)

      try 
      {
          TS_ASSERT_EQUALS(alg.execute(),true);
      }
      catch(std::runtime_error e)
      {
          TS_FAIL(e.what());
      }

      IInstrument_sptr inst = output->getInstrument();
      // get pointer to the first detector in bank 2
      boost::shared_ptr<IDetector> det = inst->getDetector(2110001);

      int id=det->getID();
      TS_ASSERT_EQUALS(2110001,id);
      V3D pos = det->getPos();
      V3D expectedPos = V3D(-1.00001,-1.51351,2.2913);
      TS_ASSERT_DELTA((pos-expectedPos).norm(),0.0,1e-5);
      //double sa=det->solidAngle(V3D(0,0,0));
      //TS_ASSERT_DELTA(sa,7.37824e-6,1e-10)
      
      // check that points lie on correct side of the scaled object
      //boost::shared_ptr<ObjComponent> pdet= boost::dynamic_pointer_cast<ObjComponent>(comp);
      //double yHW=0.00143;
      //TS_ASSERT( pdet->isValid(expectedPos) )
      //TS_ASSERT( pdet->isValid(expectedPos+V3D(0,0.99*yHW,0)) )
      //TS_ASSERT( pdet->isValid(expectedPos-V3D(0,0.99*yHW,0)) )
      // data shows scaling of 1.0195 for detector 2110001
      //TS_ASSERT( pdet->isValid(expectedPos+V3D(0,1.015*yHW,0)) )
      //TS_ASSERT( pdet->isValid(expectedPos-V3D(0,1.015*yHW,0)) )
      //TS_ASSERT( ! pdet->isValid(expectedPos+V3D(0,1.025*yHW,0)) )
      //TS_ASSERT( ! pdet->isValid(expectedPos-V3D(0,1.025*yHW,0)) )
      // check track behaves correctly
      //Track track(V3D(0,0,0),expectedPos/expectedPos.norm());
      //TS_ASSERT_EQUALS(pdet->interceptSurface(track),1)
      //Track::LType::const_iterator it=track.begin();
      //TS_ASSERT(it != track.end())
      //
      // tmp test
      //

      // get pointer to the second detector in bank 2
      boost::shared_ptr<IDetector> det2 = inst->getDetector(2110002);

      id=det2->getID();
      TS_ASSERT_EQUALS(2110002,id);
      pos = det2->getPos();
      expectedPos = V3D(-1.00001,-1.5106,2.2913);
      TS_ASSERT_DELTA((pos-expectedPos).norm(),0.0,1e-5)
      //sa=det2->solidAngle(V3D(0,0,0));
      //TS_ASSERT_DELTA(sa,7.7732e-6,1e-10)
  }

private:
  SetScalingPSD alg;
  Mantid::DataHandling::LoadRaw2 loader;
  std::string outputSpace;
  MatrixWorkspace_sptr output;

};

#endif /*SETSCALINGPSDTESTRAW_H*/
