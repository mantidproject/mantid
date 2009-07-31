#ifndef GROUPDETECTORS2TEST_H_
#define GROUPDETECTORS2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Detector.h"
#include "MantidGeometry/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <iostream>

using Mantid::DataHandling::GroupDetectors2;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class GroupDetectors2Test : public CxxTest::TestSuite
{
public:
  GroupDetectors2Test()
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Histogram1D::RCtype x,vec;
    x.access().resize(6,10.0);
    vec.access().resize(5,1.0);
    int forSpecDetMap[5];
    for (int j = 0; j < 5; ++j)
    {
      space2D->setX(j,x);
      space2D->setData(j,vec,vec);
      space2D->getAxis(1)->spectraNo(j) = j;
      forSpecDetMap[j] = j;
    }
    Detector *d = new Detector("det",0);
    d->setID(0);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d);
    Detector *d1 = new Detector("det",0);
    d1->setID(1);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d1);
    Detector *d2 = new Detector("det",0);
    d2->setID(2);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d2);
    Detector *d3 = new Detector("det",0);
    d3->setID(3);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d3);
    Detector *d4 = new Detector("det",0);
    d4->setID(4);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d4);

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, 5 );

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("GroupTestWS", space);
  }

  void testName()
  {
    TS_ASSERT_EQUALS( grouper.name(), "GroupDetectors" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( grouper.version(), 2 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( grouper.category(), "DataHandling\\Detectors" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() );

    GroupDetectors2 gd;
    TS_ASSERT_THROWS_NOTHING( gd.initialize() )
    TS_ASSERT( gd.isInitialized() );

    std::vector<Property*> props = gd.getProperties();
    TS_ASSERT_EQUALS( props.size(), 7 )

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace2D>* >(props[0]) )

    TS_ASSERT_EQUALS( props[2]->name(), "SpectraList" )
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[2]) )

    TS_ASSERT_EQUALS( props[3]->name(), "DetectorList" )
    TS_ASSERT( props[3]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[3]) )

    TS_ASSERT_EQUALS( props[4]->name(), "WorkspaceIndexList" )
    TS_ASSERT( props[4]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[4]) )
 	}

  void testExec()
  {
    if ( !grouper.isInitialized() ) grouper.initialize();

    grouper.setPropertyValue("InputWorkspace","GroupTestWS");
    grouper.setPropertyValue("OutputWorkspace","GroupTestWSOut");
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( ! grouper.isExecuted() );

    GroupDetectors2 grouper2;
    grouper2.initialize();
    grouper2.setPropertyValue("InputWorkspace","GroupTestWS");
    grouper2.setPropertyValue("OutputWorkspace","GroupTestWSOut");
    grouper2.setPropertyValue("SpectraList","0,3");
    grouper2.setProperty<bool>("KeepUngroupedSpectra",true);
    TS_ASSERT_THROWS_NOTHING( grouper2.execute());
    TS_ASSERT( grouper2.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("GroupTestWSOut"));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), 4 )
    std::vector<double> tens(6,10.0);
    std::vector<double> ones(5,1.0);
    std::vector<double> twos(5,2.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0), twos )
    for (int i = 0; i < 5; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], 1.4142, 0.0001 )
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), 0 )
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(1), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 1 )
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(2), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(2), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), 2 )
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(3), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(3), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(3), 4 )

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) )
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(1) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(2) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(3) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    
    AnalysisDataService::Instance().remove("GroupTestWS");
    AnalysisDataService::Instance().remove("GroupTestWSOut");
  }

private:
  GroupDetectors2 grouper;
};

#endif /*GROUPDETECTORSTEST_H_*/
