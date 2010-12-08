#ifndef GROUPDETECTORSTEST_H_
#define GROUPDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <iostream>

using Mantid::DataHandling::GroupDetectors;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class GroupDetectorsTest : public CxxTest::TestSuite
{
public:
  GroupDetectorsTest()
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    MantidVecPtr x,vec;
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

    Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(space->getBaseInstrument());

    Detector *d = new Detector("det",0);
    d->setID(0);
    instr->markAsDetector(d);
    Detector *d1 = new Detector("det",0);
    d1->setID(1);
    instr->markAsDetector(d1);
    Detector *d2 = new Detector("det",0);
    d2->setID(2);
    instr->markAsDetector(d2);
    Detector *d3 = new Detector("det",0);
    d3->setID(3);
    instr->markAsDetector(d3);
    Detector *d4 = new Detector("det",0);
    d4->setID(4);
    instr->markAsDetector(d4);

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
    TS_ASSERT_EQUALS( grouper.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( grouper.category(), "DataHandling\\Detectors" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() );

    GroupDetectors gd;
    TS_ASSERT_THROWS_NOTHING( gd.initialize() )
    TS_ASSERT( gd.isInitialized() );

    std::vector<Property*> props = gd.getProperties();
    TS_ASSERT_EQUALS( static_cast<int>(props.size()), 5 )

    TS_ASSERT_EQUALS( props[0]->name(), "Workspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<>* >(props[0]) )

    TS_ASSERT_EQUALS( props[1]->name(), "SpectraList" )
    TS_ASSERT( props[1]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[1]) )

    TS_ASSERT_EQUALS( props[2]->name(), "DetectorList" )
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[2]) )

    TS_ASSERT_EQUALS( props[3]->name(), "WorkspaceIndexList" )
    TS_ASSERT( props[3]->isDefault() )
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[3]) )
 	}

  void testExec()
  {
    if ( !grouper.isInitialized() ) grouper.initialize();

    grouper.setPropertyValue("Workspace","GroupTestWS");
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    grouper.setPropertyValue("WorkspaceIndexList","0,2");
    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    GroupDetectors grouper2;
    grouper2.initialize();
    grouper2.setPropertyValue("Workspace","GroupTestWS");
    grouper2.setPropertyValue("SpectraList","0,3");
    TS_ASSERT_THROWS_NOTHING( grouper2.execute());
    TS_ASSERT( grouper2.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("GroupTestWS"));
    std::vector<double> tens(6,10.0);
    std::vector<double> ones(5,1.0);
    std::vector<double> threes(5,3.0);
    std::vector<double> zeroes(5,0.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0), threes )
    for (int i = 0; i < 5; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], 1.7321, 0.0001 )
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), 0 )
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(1), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 1 )
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(2), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(2), zeroes )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), -1 )
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(3), zeroes )
    TS_ASSERT_EQUALS( outputWS->dataE(3), zeroes )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(3), -1 )
    TS_ASSERT_EQUALS( outputWS->dataX(4), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(4), ones )
    TS_ASSERT_EQUALS( outputWS->dataE(4), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(4), 4 )

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) )
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(1) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    TS_ASSERT_THROWS( outputWS->getDetector(2), Exception::NotFoundError )
    TS_ASSERT_THROWS( outputWS->getDetector(3), Exception::NotFoundError )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(4) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
	AnalysisDataService::Instance().remove("GroupTestWS");
  }

private:
  GroupDetectors grouper;
};

#endif /*GROUPDETECTORSTEST_H_*/
