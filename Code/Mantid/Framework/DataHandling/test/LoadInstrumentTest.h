#ifndef LOADINSTRUMENTTEST_H_
#define LOADINSTRUMENTTEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "MantidAPI/ExperimentInfo.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    loader.initialize();
    TS_ASSERT( loader.isInitialized() );
  }

  void testExecHET()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestHET";
    int histogramNumber = 2584;
    int timechannels = 100;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    //loop to create data
    for (int i = 0; i < histogramNumber; i++)
    {
      boost::shared_ptr<Mantid::MantidVec> timeChannelsVec(new Mantid::MantidVec),v(new Mantid::MantidVec),e(new Mantid::MantidVec);
      timeChannelsVec->resize(timechannels);
      v->resize(timechannels);
      e->resize(timechannels);
      //timechannels
      for (int j = 0; j < timechannels; j++)
      {
        (*timeChannelsVec)[j] = j*100;
        (*v)[j] = (i+j)%256;
        (*e)[j] = (i+j)%78;
      }
      // Populate the workspace.
      ws2D->setX(i, timeChannelsVec);
      ws2D->setData(i, v, e);
    }

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));
    // We want to test id the spectra mapping changes
    TS_ASSERT_EQUALS(ws2D->getSpectrum(0)->getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(ws2D->getSpectrum(256)->getSpectrumNo(), 257);
    TS_ASSERT_EQUALS(ws2D->getNumberHistograms(), 2584);
    
    loader.setPropertyValue("Filename", "HET_Definition.xml");
    inputFile = loader.getPropertyValue("Filename");
    loader.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loader.execute());

    TS_ASSERT( loader.isExecuted() );

    TS_ASSERT_EQUALS ( loader.getPropertyValue("MonitorList"), "601,602,603,604" );

//    std::vector<detid_t> dets = ws2D->getInstrument()->getDetectorIDs();
//    std::cout << dets.size() << " detectors in the instrument" << std::endl;
//    for (size_t i=0; i<dets.size(); i++)
//    {
//      if (i % 10 == 0) std::cout << std::endl;
//      std::cout << dets[i] << ", ";
//    }

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument()->baseInstrument();
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet103 = i->getDetector(103);
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);
    double d = ptrDet103->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,2.512,0.0001);
    double cmpDistance = ptrDet103->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,2.512,0.0001);

    // test if detector with det_id=603 has been marked as a monitor
    boost::shared_ptr<const IDetector> ptrMonitor = i->getDetector(601);
    TS_ASSERT( ptrMonitor->isMonitor() );

    // Spectra mapping has been updated
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(0), 1);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(255), 256);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(256), 257);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(257), 258);

    std::set<detid_t> ids_from_map = output->getSpectrum(257)->getDetectorIDs();
    IDetector_const_sptr det_from_ws = output->getDetector(257);
    TS_ASSERT_EQUALS(ids_from_map.size(), 1);
    TS_ASSERT_EQUALS(*ids_from_map.begin(), 602);
    TS_ASSERT_EQUALS(det_from_ws->getID(), 602);

    // also a few tests on the last detector and a test for the one beyond the last
    boost::shared_ptr<const IDetector> ptrDetLast = i->getDetector(413256);
    TS_ASSERT_EQUALS( ptrDetLast->getID(), 413256);
    TS_ASSERT_EQUALS( ptrDetLast->getName(), "pixel");
    TS_ASSERT_THROWS(i->getDetector(413257), Exception::NotFoundError);

    // Test input data is unchanged
    Workspace2D_sptr output2DInst = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584
    TS_ASSERT_EQUALS( output2DInst->getNumberHistograms(), histogramNumber);

    // Check running algorithm for same XML file leads to same instrument object being attached
    boost::shared_ptr<Instrument> instr(new Instrument());
    output->setInstrument(instr);
    TS_ASSERT_EQUALS( output->getInstrument()->baseInstrument(), instr );
    LoadInstrument loadAgain;
    TS_ASSERT_THROWS_NOTHING( loadAgain.initialize() );
    loadAgain.setPropertyValue("Filename", inputFile);
    loadAgain.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING( loadAgain.execute() );
    TS_ASSERT_EQUALS( output->getInstrument()->baseInstrument(), i );

    // Valid-from/to
    Kernel::DateAndTime validFrom("1900-01-31T23:59:59");
    Kernel::DateAndTime validTo("2100-01-31 23:59:59");
    TS_ASSERT_EQUALS( i->getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS( i->getValidToDate(), validTo);

    AnalysisDataService::Instance().remove(wsName);
  }


  void testExecSLS()
  {
    LoadInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestSLS";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderSLS.setPropertyValue("Filename", "SANDALS_Definition.xml");
    inputFile = loaderSLS.getPropertyValue("Filename");

    loaderSLS.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderSLS.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());

    TS_ASSERT( loaderSLS.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -11.016,0.01);

    boost::shared_ptr<const IObjComponent> samplepos = boost::dynamic_pointer_cast<const IObjComponent>(i->getSample());
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(101);
    TS_ASSERT_EQUALS( ptrDet->getID(), 101);

    boost::shared_ptr<const IDetector> ptrMonitor = i->getDetector(1);
    TS_ASSERT( ptrMonitor->isMonitor() );

    boost::shared_ptr<const IDetector> ptrDetShape = i->getDetector(102);
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.0)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.0,0.0,0.000001)+ptrDetShape->getPos()) );
    TS_ASSERT( ptrDetShape->isValid(V3D(0.005,0.1,0.000002)+ptrDetShape->getPos()) );


    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecNIMROD()
  {
    LoadInstrument loaderNIMROD;

    TS_ASSERT_THROWS_NOTHING(loaderNIMROD.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestNIMROD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderNIMROD.setPropertyValue("Filename", "NIM_Definition.xml");
    inputFile = loaderNIMROD.getPropertyValue("Filename");

    loaderNIMROD.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderNIMROD.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderNIMROD.getPropertyValue("Workspace") );
    TS_ASSERT( ! result.compare(wsName));

    TS_ASSERT_THROWS_NOTHING(loaderNIMROD.execute());

    TS_ASSERT( loaderNIMROD.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument();

    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(20201001);
    TS_ASSERT_EQUALS( ptrDet->getName(), "det 1");
    TS_ASSERT_EQUALS( ptrDet->getID(), 20201001);
    TS_ASSERT_DELTA( ptrDet->getPos().X(),  -0.0909, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Y(), 0.3983, 0.0001);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(),  4.8888, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecHRP2()
  {
    // Test Parameter file in instrument folder is used by an IDF file not in the instrument folder
    doTestParameterFileSelection("IDFs_for_UNIT_TESTING/HRPD_Definition.xml","HRPD_Parameters.xml","S"); 
  }

  void testExecHRP3()
  {
    // Test Parameter file in instrument folder is used by an IDF file not in the instrument folder and
    // with an extension of its name after the 'Definition' not present in a parameter file.
    doTestParameterFileSelection("IDFs_for_UNIT_TESTING/HRPD_Definition_Test3.xml","HRPD_Parameters.xml","S"); 
  }

  void testExecHRP4()
  {
    // Test Parameter file outside of instrument folder is used by an IDF file in the same folder and
    // with the same extension ('_Test4') of its name after the 'Definition' or 'Parameter'.
    doTestParameterFileSelection("IDFs_for_UNIT_TESTING/HRPD_Definition_Test4.xml","IDFs_for_UNIT_TESTING/HRPD_Parameters_Test4.xml","T"); 
  }

  void testExecHRP5()
  {
    // Test Parameter file outside instrument folder is used by an IDF file in the same folder
    doTestParameterFileSelection("IDFs_for_UNIT_TESTING/HRPDTEST_Definition.xml","IDFs_for_UNIT_TESTING/HRPDTEST_Parameters.xml","U"); 
  }

  void testExecHRP6()
  {
    // Test Parameter file outside of instrument folder is used by an IDF file in the same folder and
    // with the same extension ('_Test6') of its name after the 'Definition' or 'Parameter' 
    // even though there is a definition file without an extension in the same folder.
    doTestParameterFileSelection("IDFs_for_UNIT_TESTING/HRPDTEST_Definition_Test6.xml","IDFs_for_UNIT_TESTING/HRPDTEST_Parameters_Test6.xml","V"); 
  }

  void testExecHRP7()
  {
    // Test Parameter file outside instrument folder is used by an IDF file in same instrument folder and
    // with an extension of its name after the 'Definition' not present in a parameter file.
    doTestParameterFileSelection("IDFs_for_UNIT_TESTING/HRPDTEST_Definition_Test7.xml","HRPDTEST_Parameters.xml","U"); 
  }

  void testNeutronicPositions()
  {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl& IDS = InstrumentDataService::Instance();
    IDS.clear();

    LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/INDIRECT_Definition.xml");
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    loader.setProperty("Workspace", ws );
    TS_ASSERT( loader.execute() );

    // This kind of IDF should lead to 2 instrument definitions - the physical and the neutronic
    // But only 1 goes into the IDS (the neutronic instrument holds the physical instrument within itself)
    TS_ASSERT_EQUALS( IDS.size(), 1 );
    if (IDS.size() != 1) return;
    TS_ASSERT_EQUALS( IDS.getObjects()[0]->getName(), "INDIRECT");
    

    // Retrieve the neutronic instrument from the InstrumentDataService
    Instrument_const_sptr neutronicInst = IDS.getObjects()[0];
    // And pull out a handle to the physical instrument from within the neutronic one
    Instrument_const_sptr physicalInst = neutronicInst->getPhysicalInstrument();
    // They should not be the same object
    TS_ASSERT_DIFFERS( physicalInst.get(), neutronicInst.get() );
    // Not true in general, but in this case we should not be getting a paramaterized instrument
    TS_ASSERT( ! physicalInst->isParametrized() );

    // Check the positions of the 6 detectors in the physical instrument
    TS_ASSERT_EQUALS( physicalInst->getDetector(1000)->getPos(), V3D(0,0,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1001)->getPos(), V3D(0,1,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1002)->getPos(), V3D(1,0,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1003)->getPos(), V3D(1,1,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1004)->getPos(), V3D(2,0,0) );
    TS_ASSERT_EQUALS( physicalInst->getDetector(1005)->getPos(), V3D(2,1,0) );

    // Check the right instrument ended up on the workspace
    TS_ASSERT_EQUALS( neutronicInst.get(), ws->getInstrument()->baseInstrument().get() );
    // Check the neutronic positions
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1000)->getPos(), V3D(2,2,0) );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1001)->getPos(), V3D(2,3,0) );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1002)->getPos(), V3D(3,2,0) );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1003)->getPos(), V3D(3,3,0) );
    // Note that one of the physical pixels doesn't exist in the neutronic space
    TS_ASSERT_THROWS( neutronicInst->getDetector(1004), Exception::NotFoundError );
    TS_ASSERT_EQUALS( neutronicInst->getDetector(1005)->getPos(), V3D(4,3,0) );

    // Check that the first 2 detectors share the same shape in the physical instrument...
    TS_ASSERT_EQUALS( physicalInst->getDetector(1000)->shape(), physicalInst->getDetector(1001)->shape() )
    // ...but not in the neutronic instrument
    TS_ASSERT_DIFFERS( neutronicInst->getDetector(1000)->shape(), neutronicInst->getDetector(1001)->shape() )
    // Also, the same shape is shared between the corresponding '1000' detectors
    TS_ASSERT_EQUALS( physicalInst->getDetector(1000)->shape(), neutronicInst->getDetector(1000)->shape() )

    // Check the monitor is in the same place in each instrument
    TS_ASSERT_EQUALS( physicalInst->getDetector(1)->getPos(), neutronicInst->getDetector(1)->getPos() );
    // ...but is not the same object
    TS_ASSERT_DIFFERS( physicalInst->getDetector(1).get(), neutronicInst->getDetector(1).get() );

    // Clean up
    IDS.clear();
  }

  void test_loading_via_InstrumentXML_property()
  {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl& IDS = InstrumentDataService::Instance();
    IDS.clear();

    // Minimal XML instrument, inspired by IDF_for_UNIT_TESTING3.xml
    const std::string instrumentXML =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
        "<instrument name=\"xmlInst\" valid-from=\"1900-01-31 23:59:59\" valid-to=\"2100-01-31 23:59:59\" last-modified=\"2010-10-06T16:21:30\">"
        "<defaults />"
        "<component type=\"panel\" idlist=\"idlist_for_bank1\">"
            "<location r=\"0\" t=\"0\" rot=\"0\" axis-x=\"0\" axis-y=\"1\" axis-z=\"0\" name=\"bank1\" xpixels=\"3\" ypixels=\"2\" />"
        "</component>"
        "<type is=\"detector\" name=\"panel\">"
          "<properties/>"
          "<component type=\"pixel\">"
            "<location y=\"1\" x=\"1\"/>"
          "</component>"
        "</type>"
        "<type is=\"detector\" name=\"pixel\">"
          "<cuboid id=\"pixel-shape\" />"
          "<algebra val=\"pixel-shape\"/>"
        "</type>"
        "<idlist idname=\"idlist_for_bank1\">"
          "<id start=\"1005\" end=\"1005\" />"
        "</idlist>"
        "</instrument>";

    LoadInstrument instLoader;
    instLoader.setRethrows(true);
    instLoader.initialize();
    instLoader.setProperty("Workspace",WorkspaceFactory::Instance().create("EventWorkspace",1,1,1));
    instLoader.setProperty("InstrumentXML",instrumentXML);
    instLoader.setProperty("InstrumentName", "Nonsense"); // Want to make sure it doesn't matter what we call it

    instLoader.execute();

    TS_ASSERT_EQUALS(1, IDS.size())
  }

  void test_failure_if_InstrumentXML_property_set_but_not_InstrumentName()
  {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setProperty("Workspace",WorkspaceFactory::Instance().create("EventWorkspace",1,1,1));
    instLoader.setProperty("InstrumentXML","<doesn't matter what>");

    TS_ASSERT( ! instLoader.execute() )
  }

  void test_failure_if_InstrumentXML_is_malformed()
  {
    LoadInstrument instLoader;
    instLoader.initialize();
    instLoader.setProperty("Workspace",WorkspaceFactory::Instance().create("EventWorkspace",1,1,1));
    instLoader.setProperty("InstrumentXML","<instrument>");
    instLoader.setProperty("InstrumentName", "Nonsense");

    TS_ASSERT( ! instLoader.execute() )
  }

  void test_loading_default_view()
  {
    // Make sure the IDS is empty
    InstrumentDataServiceImpl& IDS = InstrumentDataService::Instance();
    IDS.clear();

    // Minimal XML instrument
    const std::string instrumentXML =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
        "<instrument name=\"xmlInst\" valid-from=\"1900-01-31 23:59:59\" valid-to=\"2100-01-31 23:59:59\" last-modified=\"2010-10-06T16:21:30\">"
        "<defaults>"
        "<!-- view -->"
        "</defaults>"
       
        "<component type=\"panel\" idlist=\"idlist_for_bank1\">"
            "<location r=\"0\" t=\"0\" rot=\"0\" axis-x=\"0\" axis-y=\"1\" axis-z=\"0\" name=\"bank1\" xpixels=\"3\" ypixels=\"2\" />"
        "</component>"
        "<type is=\"detector\" name=\"panel\">"
          "<properties/>"
          "<component type=\"pixel\">"
            "<location y=\"1\" x=\"1\"/>"
          "</component>"
        "</type>"
        "<type is=\"detector\" name=\"pixel\">"
          "<cuboid id=\"pixel-shape\" />"
          "<algebra val=\"pixel-shape\"/>"
        "</type>"
        "<idlist idname=\"idlist_for_bank1\">"
          "<id start=\"1005\" end=\"1005\" />"
        "</idlist>"
        "</instrument>";

    LoadInstrument instLoader;
    instLoader.setRethrows(true);
    instLoader.initialize();
    instLoader.setProperty("Workspace",WorkspaceFactory::Instance().create("EventWorkspace",1,1,1));
    instLoader.setProperty("InstrumentXML",instrumentXML);
    instLoader.setProperty("InstrumentName", "Nonsense"); // Want to make sure it doesn't matter what we call it

    instLoader.execute();

    TS_ASSERT_EQUALS(1, IDS.size());
    // test that the default default view is "3D"
    auto instr = IDS.getObjects().front();
    TS_ASSERT_EQUALS( instr->getDefaultView(), "3D" );
    IDS.clear();

    // explicitely set the default instrument view
    const std::string instrumentXMLwithView = Mantid::Kernel::Strings::replace(instrumentXML,
    "<!-- view -->", "<default-view view=\"cylindrical_y\"/>");

    instLoader.setProperty("Workspace",WorkspaceFactory::Instance().create("EventWorkspace",1,1,1));
    instLoader.setProperty("InstrumentXML",instrumentXMLwithView);
    instLoader.setProperty("InstrumentName", "Nonsense"); // Want to make sure it doesn't matter what we call it

    instLoader.execute();

    TS_ASSERT_EQUALS(1, IDS.size());
    // test that the default view is cylindrical_y
    instr = IDS.getObjects().front();
    TS_ASSERT_EQUALS( instr->getDefaultView(), "CYLINDRICAL_Y" );
    IDS.clear();

  }

private:

  // @param filename Filename to an IDF
  // @param paramFilename Expected parameter file to be loaded as part of LoadInstrument
  // @param par A specific parameter to check if have been loaded
  void doTestParameterFileSelection(std::string filename, std::string paramFilename, std::string par )
  {
    InstrumentDataService::Instance().clear();

    LoadInstrument loader;

    TS_ASSERT_THROWS_NOTHING(loader.initialize());

    //create a workspace with some sample data
    wsName = "LoadInstrumentTestForParameterFileSelection";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // load IDF
    loader.setPropertyValue("Filename", filename);
    inputFile = loader.getPropertyValue("Filename");
    loader.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );


    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument();

    // test if a dummy parameter has been read in
    boost::shared_ptr<const IComponent> comp = i->getComponentByName("bank_90degnew");
    TS_ASSERT_EQUALS( comp->getName(), "bank_90degnew");

    ParameterMap& paramMap = output->instrumentParameters();

    // It's "X0" in parameter file IDFs_for_UNIT_TESTING/HRPD_Parameters_Test4.xml
    Parameter_sptr param = paramMap.getRecursive(&(*comp), par, "fitting");
    TS_ASSERT( param );
    if( param != 0) {
       const FitParameter& fitParam4 = param->value<FitParameter>();
       TS_ASSERT( fitParam4.getTie().compare("") == 0 );
       TS_ASSERT( fitParam4.getFunction().compare("BackToBackExponential") == 0 );
    }
    else
    {
      TS_FAIL( "Did not select "+paramFilename+" for "+filename); 
    }

    AnalysisDataService::Instance().remove(wsName);

  }

  LoadInstrument loader;
  std::string inputFile;
  std::string wsName;
};



class LoadInstrumentTestPerformance : public CxxTest::TestSuite
{
public:
  MatrixWorkspace_sptr ws;

  void setUp()
  {
    ws = WorkspaceCreationHelper::Create2DWorkspace(1,2);
  }

  void doTest(std::string filename, size_t numTimes = 1)
  {
    for (size_t i=0; i < numTimes; ++i)
    {
      // Remove any existing instruments, so each time they are loaded.
      InstrumentDataService::Instance().clear();
      // Load it fresh
      LoadInstrument loader;
      loader.initialize();
      loader.setProperty("Workspace", ws);
      loader.setPropertyValue("Filename", filename);
      loader.execute();
      TS_ASSERT( loader.isExecuted() );
    }
  }

  void test_GEM()
  {
    doTest("GEM_Definition.xml", 10);
  }

  void test_WISH()
  {
    doTest("WISH_Definition.xml", 1);
  }

  void test_BASIS()
  {
    doTest("BASIS_Definition_0-20130119.xml", 5);
  }

  void test_CNCS()
  {
    doTest("CNCS_Definition.xml", 5);
  }

  void test_SEQUOIA()
  {
    doTest("SEQUOIA_Definition.xml", 5);
  }

  void test_POWGEN_2011()
  {
    doTest("POWGEN_Definition_2011-02-25.xml", 10);
  }

  void test_TOPAZ_2010()
  {
    doTest("TOPAZ_Definition_2010.xml", 1);
  }

  void test_TOPAZ_2011()
  {
    doTest("TOPAZ_Definition_2011-01-01.xml", 1);
  }

  void test_SNAP()
  {
    doTest("SNAP_Definition.xml", 1);
  }

};


#endif /*LOADINSTRUMENTTEST_H_*/

