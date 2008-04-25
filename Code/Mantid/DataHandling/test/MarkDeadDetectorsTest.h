#ifndef MARKDEADDETECTORSTEST_H_
#define MARKDEADDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/MarkDeadDetectors.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Detector.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class MarkDeadDetectorsTest : public CxxTest::TestSuite
{
public:
  MarkDeadDetectorsTest()
  {
    FrameworkManager::Instance().initialize();
    
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    std::vector<double> x(6,10.0);
    std::vector<double>  vec(5,1.0);
    for (int j = 0; j < 5; ++j) 
    {
      space2D->setX(j,x);
      space2D->setData(j,vec,vec);
      space2D->spectraNo(j) = j;
    }
    Detector *d = new Detector;
    d->setID(0);
    space->getInstrument().markAsDetector(d);
    Detector *d1 = new Detector;
    d1->setID(1);
    space->getInstrument().markAsDetector(d1);
    Detector *d2 = new Detector;
    d2->setID(2);
    space->getInstrument().markAsDetector(d2);
    Detector *d3 = new Detector;
    d3->setID(3);
    space->getInstrument().markAsDetector(d3);
    Detector *d4 = new Detector;
    d4->setID(4);
    space->getInstrument().markAsDetector(d4);
      
    // Register the workspace in the data service
    AnalysisDataService::Instance().add("testSpace", space);
  }
  
  void testName()
  {
    TS_ASSERT_EQUALS( marker.name(), "MarkDeadDetectors" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( marker.version(), 1 )
  }

  void testInit()
  {    
    TS_ASSERT_THROWS_NOTHING( marker.initialize() )
    TS_ASSERT( marker.isInitialized() );

    MarkDeadDetectors mdd;
    TS_ASSERT_THROWS_NOTHING( mdd.initialize() )
    TS_ASSERT( mdd.isInitialized() );
    
    std::vector<Property*> props = mdd.getProperties();
    
    TS_ASSERT_EQUALS( props[0]->name(), "Workspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace2D>* >(props[0]) )
    
    TS_ASSERT_EQUALS( props[1]->name(), "WorkspaceIndexList" )
    TS_ASSERT( props[1]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[1]) )
    
    TS_ASSERT_EQUALS( props[2]->name(), "WorkspaceIndexMin" )
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<int>* >(props[2]) )

    TS_ASSERT_EQUALS( props[3]->name(), "WorkspaceIndexMax" )
    TS_ASSERT( props[3]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<int>* >(props[3]) )
  }
  
  void testExec()
  {
    if ( !marker.isInitialized() ) marker.initialize();

    marker.setPropertyValue("Workspace","testSpace");
    
    TS_ASSERT_THROWS_NOTHING( marker.execute());
    TS_ASSERT( marker.isExecuted() );
    
    marker.setPropertyValue("WorkspaceIndexList","0,3");
    TS_ASSERT_THROWS_NOTHING( marker.execute());
    Workspace_sptr outputWS = AnalysisDataService::Instance().retrieve("testSpace");
    std::vector<double> tens(6,10.0);
    std::vector<double> ones(5,1.0);
    std::vector<double> zeroes(5,0.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(0), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(1), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones )
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(2), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(2), ones )
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(3), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(3), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataX(4), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(4), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(4), ones )
    Instrument& i = outputWS->getInstrument();
    TS_ASSERT( i.getDetector(0)->isDead() )
    TS_ASSERT( ! i.getDetector(1)->isDead() )
    TS_ASSERT( ! i.getDetector(2)->isDead() )
    TS_ASSERT( i.getDetector(3)->isDead() )
    TS_ASSERT( ! i.getDetector(4)->isDead() )
    
    marker.setPropertyValue("WorkspaceIndexMin","2");
    // Should cope with me setting this to a high value
    marker.setPropertyValue("WorkspaceIndexMax","8");
    TS_ASSERT_THROWS_NOTHING( marker.execute());
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(0), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(1), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones )
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(2), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(2), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(3), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(3), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataX(4), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(4), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(4), zeroes )
    TS_ASSERT( i.getDetector(0)->isDead() )
    TS_ASSERT( ! i.getDetector(1)->isDead() )
    TS_ASSERT( i.getDetector(2)->isDead() )
    TS_ASSERT( i.getDetector(3)->isDead() )
    TS_ASSERT( i.getDetector(4)->isDead() )
  }
  
private:
  MarkDeadDetectors marker;
};

#endif /*MARKDEADDETECTORSTEST_H_*/
