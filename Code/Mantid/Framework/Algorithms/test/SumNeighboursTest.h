#ifndef SumNeighboursTEST_H_
#define SumNeighboursTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SumNeighbours.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
//#include "MantidDataHandling/LoadEventNexus.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SumNeighboursTest : public CxxTest::TestSuite
{
public:

  SumNeighboursTest()
  {
//    outputSpace1 = "SNAP_sum";
//    inputSpace = "SNAP";
//
//    Mantid::NeXus::LoadSNSEventNexus loader;
//    loader.initialize();
//    loader.setPropertyValue("Filename","/home/janik/data/SNAP_4105_event.nxs");
//    loader.setPropertyValue("OutputWorkspace",inputSpace);
//    loader.execute();

  }

  ~SumNeighboursTest()
  {}



  void testTheBasics()
  {
    TS_ASSERT_EQUALS( alg.name(), "SumNeighbours" );
    TS_ASSERT_EQUALS( alg.version(), 1 );
    TS_ASSERT_EQUALS( alg.category(), "General" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    std::vector<Property*> props = alg.getProperties();
    TS_ASSERT_EQUALS( static_cast<int>(props.size()), 8 );
  }


//
//
//  void testExec_SNAP()
//  {
//    alg.initialize();
//    TS_ASSERT( alg.isInitialized() );
//
//    // Set the properties
//    alg.setPropertyValue("InputWorkspace",inputSpace) ;
//    alg.setPropertyValue("OutputWorkspace",outputSpace1) ;
//    alg.setPropertyValue("SumX","4");
//    alg.setPropertyValue("SumY","16");
//    //alg.setPropertyValue("DetectorNames","bank1,bank2,bank3,bank4,bank5,bank6,bank7,bank8,bank9,bank10,bank11,bank12,bank13,bank14,bank15,bank16,bank17,bank18");
//    //alg.setPropertyValue("DetectorNames","bank1,bank2,bank3");
//
//    alg.execute();
//
//    //TS_ASSERT_EQUALS(
//  }


private:
  SumNeighbours alg;   // Test with range limits
  std::string outputSpace1;
  std::string outputSpace2;
  std::string inputSpace;
};

#endif /*SumNeighboursTEST_H_*/


