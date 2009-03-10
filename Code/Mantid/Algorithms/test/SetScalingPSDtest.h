#ifndef SETSCALINGPSDTEST_H_
#define SETSCALINGPSDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidAlgorithms/SetScalingPSD.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
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
    //std::string inputFile = "../../../../Test/Data/HET15869.RAW";
    std::string inputFile = "../../../../Test/Data/MER02257.raw";
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "testWS";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    //
    // get workspace
    //
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)) );
    //Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
   }

  void testLoadScalingFile()
  {
    //loader.initialize();
    //loader.setPropertyValue("ScalingFilename", "./scalingtest.sca");
    //loader.setPropertyValue("OutputWorkspace", "EMU6473");
    //TS_ASSERT_THROWS_NOTHING( loader.execute() );
    //TS_ASSERT_EQUALS(loader.isExecuted(),true);

    //alg.setPropertyValue("ScalingFileName", "merlin.sca");
    alg.setPropertyValue("ScalingFileName", "../../../../Test/Data/merlin_detector.sca");
    alg.setPropertyValue("Workspace", "testWS");
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = alg.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare("testWS"));
  }

  void testExecute()
  {
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
      boost::shared_ptr<IComponent> 
             comp = (*boost::dynamic_pointer_cast<ICompAssembly>(
                       (*boost::dynamic_pointer_cast<ICompAssembly>(
                          (*boost::dynamic_pointer_cast<ICompAssembly>(
                                (*boost::dynamic_pointer_cast<ICompAssembly>(inst))[3]))[0]))[0]))[0];

      boost::shared_ptr<IDetector> det = boost::dynamic_pointer_cast<IDetector>(comp);
      int id=det->getID();
      TS_ASSERT_EQUALS(2110001,id);
      V3D pos = det->getPos();
      V3D expectedPos = V3D(-1.000004,-1.5145256,2.291291);
      TS_ASSERT_DELTA((pos-expectedPos).norm(),0.0,1e-5)
  }

private:
  SetScalingPSD alg;
  Mantid::DataHandling::LoadRaw2 loader;
  std::string outputSpace;
  MatrixWorkspace_sptr output;

};

#endif /*SETSCALINGPSDTEST_H_*/
