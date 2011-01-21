#ifndef MARKDEADDETECTORSTEST_H_
#define MARKDEADDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/MaskDetectors.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::MantidVecPtr;

class MaskDetectorsTest : public CxxTest::TestSuite
{
public:

  static MaskDetectorsTest *createSuite() { return new MaskDetectorsTest(); }
  static void destroySuite(MaskDetectorsTest *suite) { delete suite; }

  MaskDetectorsTest()
  {
  }

  void testName()
  {
    TS_ASSERT_EQUALS( marker.name(), "MaskDetectors" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( marker.version(), 1 );
  }


  void setUpWS(bool event, const std::string & name = "testSpace")
  {
    MatrixWorkspace_sptr space;
    int forSpecDetMap[5];

    // Set up a small workspace for testing
    if (event)
    {
      space = WorkspaceFactory::Instance().create("EventWorkspace",5,6,5);
      EventWorkspace_sptr spaceEvent = boost::dynamic_pointer_cast<EventWorkspace>(space);

      MantidVecPtr x,vec;
      vec.access().resize(5,1.0);
      for (int j = 0; j < 5; ++j)
      {
        //Just one event per pixel
        TofEvent event(1.23, 4.56);
        spaceEvent->getEventListAtPixelID(j).addEventQuickly(event);
        spaceEvent->getAxis(1)->spectraNo(j) = j;
        forSpecDetMap[j] = j;
      }

      spaceEvent->doneLoadingData();
      x.access().push_back(0.0);
      x.access().push_back(10.0);
      spaceEvent->setAllX(x);

    }
    else
    {
      space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
      Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);

      MantidVecPtr x,vec;
      x.access().resize(6,10.0);
      vec.access().resize(5,1.0);
      for (int j = 0; j < 5; ++j)
      {
        space2D->setX(j,x);
        space2D->setData(j,vec,vec);
        space2D->getAxis(1)->spectraNo(j) = j;
        forSpecDetMap[j] = j;
      }
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
    AnalysisDataService::Instance().add(name, space);

  }

  //---------------------------------------------------------------------------------------------
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( marker.initialize() );
    TS_ASSERT( marker.isInitialized() );

    MaskDetectors mdd;
    TS_ASSERT_THROWS_NOTHING( mdd.initialize() );
    TS_ASSERT( mdd.isInitialized() );

    std::vector<Property*> props = mdd.getProperties();
    TS_ASSERT_EQUALS( static_cast<int>(props.size()), 5 );

    TS_ASSERT_EQUALS( props[0]->name(), "Workspace" );
    TS_ASSERT( props[0]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<>* >(props[0]) );

    TS_ASSERT_EQUALS( props[1]->name(), "SpectraList" );
    TS_ASSERT( props[1]->isDefault() );
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[1]) );

    TS_ASSERT_EQUALS( props[2]->name(), "DetectorList" );
    TS_ASSERT( props[2]->isDefault() );
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[2]) );

    TS_ASSERT_EQUALS( props[3]->name(), "WorkspaceIndexList" );
    TS_ASSERT( props[3]->isDefault() );
    TS_ASSERT( dynamic_cast<ArrayProperty<int>* >(props[3]) );

    TS_ASSERT_EQUALS( props[4]->name(), "MaskedWorkspace" );
    TS_ASSERT( props[4]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<>* >(props[4]) );
  }

  void testExecWithNoInput()
  {
    setUpWS(false);

    MaskDetectors masker;
    
    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("Workspace","testSpace"));

    TS_ASSERT_THROWS_NOTHING(masker.execute());

    AnalysisDataService::Instance().remove("testSpace");
  }

  //---------------------------------------------------------------------------------------------
  void testExec()
  {
    setUpWS(false);

    if ( !marker.isInitialized() ) marker.initialize();

    marker.setPropertyValue("Workspace","testSpace");

    TS_ASSERT_THROWS_NOTHING(marker.execute());
    TS_ASSERT( marker.isExecuted() );

    marker.setPropertyValue("WorkspaceIndexList","0,3");
    TS_ASSERT_THROWS_NOTHING( marker.execute());

    MaskDetectors marker2;
    marker2.initialize();
    marker2.setPropertyValue("Workspace","testSpace");
    marker2.setPropertyValue("SpectraList","2");
    TS_ASSERT_THROWS_NOTHING( marker2.execute());
    TS_ASSERT( marker2.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testSpace"));
    std::vector<double> tens(6,10.0);
    std::vector<double> ones(5,1.0);
    std::vector<double> zeroes(5,0.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataE(0), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(1), ones );
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones );
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(2), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataE(2), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(3), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataE(3), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataX(4), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(4), ones );
    TS_ASSERT_EQUALS( outputWS->dataE(4), ones );
    boost::shared_ptr<IInstrument> i = outputWS->getInstrument();
    TS_ASSERT( i->getDetector(0)->isMasked() );
    TS_ASSERT( ! i->getDetector(1)->isMasked() );
    TS_ASSERT( i->getDetector(2)->isMasked() );
    TS_ASSERT( i->getDetector(3)->isMasked() );
    TS_ASSERT( ! i->getDetector(4)->isMasked() );

    AnalysisDataService::Instance().remove("testSpace");
  }


  //---------------------------------------------------------------------------------------------
  void testExecEventWorkspace()
  {
    setUpWS(true);

    if ( !marker.isInitialized() ) marker.initialize();

    marker.setPropertyValue("Workspace","testSpace");

    TS_ASSERT_THROWS_NOTHING( marker.execute());
    TS_ASSERT( marker.isExecuted() );

    marker.setPropertyValue("WorkspaceIndexList","0,3");
    TS_ASSERT_THROWS_NOTHING( marker.execute());

    MaskDetectors marker2;
    marker2.initialize();
    marker2.setPropertyValue("Workspace","testSpace");
    marker2.setPropertyValue("SpectraList","2");
    TS_ASSERT_THROWS_NOTHING( marker2.execute());
    TS_ASSERT( marker2.isExecuted() );

    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testSpace"));
    std::vector<double> tens;
    tens.push_back(0.0);
    tens.push_back(10.0);
    std::vector<double> ones(1,1.0);
    std::vector<double> zeroes(1,0.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataE(0), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(1), ones );
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones );
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(2), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataE(2), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(3), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataE(3), zeroes );
    TS_ASSERT_EQUALS( outputWS->dataX(4), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(4), ones );
    TS_ASSERT_EQUALS( outputWS->dataE(4), ones );
    boost::shared_ptr<IInstrument> i = outputWS->getInstrument();
    TS_ASSERT( i->getDetector(0)->isMasked() );
    TS_ASSERT( ! i->getDetector(1)->isMasked() );
    TS_ASSERT( i->getDetector(2)->isMasked() );
    TS_ASSERT( i->getDetector(3)->isMasked() );
    TS_ASSERT( ! i->getDetector(4)->isMasked() );

  AnalysisDataService::Instance().remove("testSpace");
  }

  void test_That_Giving_A_Workspace_Containing_Masks_Copies_These_Masks_Over()
  {
    // Create 2 workspaces
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    setUpWS(false, inputWSName);
    setUpWS(false, existingMaskName);
    MatrixWorkspace_sptr existingMask = 
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(existingMaskName));

    // Mask some detectors on the existing mask workspace
    std::set<int> masked_indices;
    masked_indices.insert(0);
    masked_indices.insert(3);
    masked_indices.insert(4);
    
    ParameterMap & pmap = existingMask->instrumentParameters();
    for( int i = 0; i < existingMask->getNumberHistograms(); ++i )
    {
      if( masked_indices.count(i) == 1 )
      {
	IDetector_sptr det;
	TS_ASSERT_THROWS_NOTHING(det = existingMask->getDetector(i));
	pmap.addBool(det.get(), "masked", true);	
      }
    }

    MaskDetectors masker;
    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    
    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("Workspace", inputWSName));
    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("MaskedWorkspace", existingMaskName));
    
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());

    //Test the original has the correct spectra masked
    MatrixWorkspace_sptr originalWS = 
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWSName));

    TS_ASSERT(originalWS);
    if( !originalWS ) return;
    for( int i = 0; i < originalWS->getNumberHistograms(); ++i )
    {
      IDetector_sptr det;
      TS_ASSERT_THROWS_NOTHING(det = existingMask->getDetector(i));
      if( masked_indices.count(i) == 1 )
      {
	TS_ASSERT_EQUALS(det->isMasked(), true);
	TS_ASSERT_EQUALS(originalWS->readY(i)[0], 0.0);
      }
      else
      {
	TS_ASSERT_EQUALS(det->isMasked(), false);
	TS_ASSERT_EQUALS(originalWS->readY(i)[0], 1.0);
      }
    }
    
    //Cleanup
    AnalysisDataService::Instance().remove(inputWSName);
    AnalysisDataService::Instance().remove(existingMaskName);
  }


private:
  MaskDetectors marker;
};

#endif /*MARKDEADDETECTORSTEST_H_*/
