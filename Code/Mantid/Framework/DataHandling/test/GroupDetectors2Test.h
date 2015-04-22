#ifndef GROUPDETECTORS2TEST_H_
#define GROUPDETECTORS2TEST_H_

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iostream>
#include <numeric>
#include <Poco/Path.h>
#include "MantidDataHandling/MaskDetectors.h"

using Mantid::DataHandling::GroupDetectors2;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::detid_t;

class GroupDetectors2Test : public CxxTest::TestSuite
{
public:

  static GroupDetectors2Test *createSuite() { return new GroupDetectors2Test(); }
  static void destroySuite(GroupDetectors2Test *suite) { delete suite; }
  
  GroupDetectors2Test() :
      inputWS("groupdetectorstests_input_workspace"),
        outputBase("groupdetectorstests_output_basename"),
        inputFile(Poco::Path::current()+"GroupDetectors2Test_mapfile_example")
  {
    // This is needed to load in the plugin algorithms (specifically Divide, which is a Child Algorithm of GroupDetectors)
    FrameworkManager::Instance();
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space =
      WorkspaceFactory::Instance().create("Workspace2D", NHIST, NBINS+1, NBINS);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Mantid::MantidVecPtr xs, errors, data[NHIST];
    xs.access().resize(NBINS+1, 10.0);
    errors.access().resize(NBINS, 1.0);
    for (int j = 0; j < NHIST; ++j)
    {
      space2D->setX(j,xs);
      data[j].access().resize(NBINS, j + 1);  // the y values will be different for each spectra (1+index_number) but the same for each bin
      space2D->setData(j, data[j], errors);
      space2D->getSpectrum(j)->setSpectrumNo(j+1);  // spectra numbers are also 1 + index_numbers because this is the tradition
      space2D->getSpectrum(j)->setDetectorID(j);
    }

    Instrument_sptr instr(new Instrument);
    for (detid_t i=0; i<6; i++)
    {
      Detector *d = new Detector("det", i,0);
      instr->markAsDetector(d);
    }
    space->setInstrument(instr);

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(inputWS, space);
  }
  
  ~GroupDetectors2Test()
  {
    AnalysisDataService::Instance().remove(inputWS);
  }

  void testSetup()
  {
    GroupDetectors2 gd;
    TS_ASSERT_EQUALS( gd.name(), "GroupDetectors" );
    TS_ASSERT_EQUALS( gd.version(), 2 );
    TS_ASSERT_THROWS_NOTHING( gd.initialize() );
    TS_ASSERT( gd.isInitialized() );

    gd.setPropertyValue("InputWorkspace", inputWS);
    gd.setPropertyValue("OutputWorkspace", outputBase);
    TS_ASSERT_THROWS_NOTHING( gd.execute());
    TS_ASSERT( ! gd.isExecuted() );
    
    AnalysisDataService::Instance().remove(outputBase);
  }

  void testAveragingWithNoInstrument()
  {
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspace123(3,3,false);
    GroupDetectors2 grouper;
    grouper.initialize();
    grouper.setChild(true);
    grouper.setProperty("InputWorkspace", testWS);
    grouper.setPropertyValue("OutputWorkspace", "__anonymous");
    grouper.setPropertyValue("WorkspaceIndexList", "0,1,2");
    grouper.setPropertyValue("Behaviour", "Average");
    TS_ASSERT_THROWS_NOTHING(grouper.execute());

    MatrixWorkspace_sptr outputWS = grouper.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
    for(size_t i = 0; i < 3; ++i)
    {
      TS_ASSERT_DELTA(outputWS->readY(0)[0], 2.0, 1e-12);
    }

  }

  void testSpectraList()
  {
    GroupDetectors2 grouper3;
    grouper3.initialize();
    grouper3.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "Specs");
    grouper3.setPropertyValue("OutputWorkspace", output);
    grouper3.setPropertyValue("SpectraList","1,4");
    // if you change the default for KeepUngrou... then uncomment what follows    grouper3.setProperty<bool>("KeepUngroupedSpectra",false);
    TS_ASSERT_THROWS_NOTHING( grouper3.execute());
    TS_ASSERT( grouper3.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), 1 );
    std::vector<double> tens(NBINS+1, 10.0);
    std::vector<double> ones(NBINS, 1.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(NBINS, 1+4) );
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], std::sqrt(double(2)), 0.0001 );
    }

    boost::shared_ptr<const IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<const DetectorGroup>(det) );
    TS_ASSERT_THROWS_ANYTHING(det = outputWS->getDetector(1));
    
    AnalysisDataService::Instance().remove(output);
  }
  void testIndexList()
  {
    GroupDetectors2 grouper3;
    grouper3.initialize();
    grouper3.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "Indices");
    grouper3.setPropertyValue("OutputWorkspace", output);

    // test the algorithm behaves if you give it a non-existent index
    grouper3.setPropertyValue("WorkspaceIndexList","4-6");
    grouper3.execute();
    TS_ASSERT( ! grouper3.isExecuted() );

    grouper3.setPropertyValue("WorkspaceIndexList","2-5");
    TS_ASSERT_THROWS_NOTHING( grouper3.execute());
    TS_ASSERT( grouper3.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), 1 );
    std::vector<double> tens(NBINS+1, 10.0);
    std::vector<double> ones(NBINS, 1.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(NBINS, (3+4+5+6)) );
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], std::sqrt(4.0), 0.0001 );
    }

    boost::shared_ptr<const IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<const DetectorGroup>(det) );
    TS_ASSERT_THROWS_ANYTHING(det = outputWS->getDetector(1));
    
    AnalysisDataService::Instance().remove(output);
  }

  void testDetectorList()
  {
    GroupDetectors2 grouper3;
    grouper3.initialize();
    grouper3.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "Detects");
    grouper3.setPropertyValue("OutputWorkspace", output);
    grouper3.setPropertyValue("DetectorList","3,1,4,0,2,5");
    grouper3.setProperty<bool>("KeepUngroupedSpectra", true);

    TS_ASSERT_THROWS_NOTHING( grouper3.execute());
    TS_ASSERT( grouper3.isExecuted() );

    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), 1 );
    std::vector<double> tens(NBINS+1, 10.0);
    std::vector<double> ones(NBINS, 1.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0),
      std::vector<double>(NBINS, (3+1)+(1+1)+(4+1)+(0+1)+(2+1)+(5+1)) );
    for (int i = 0; i < NBINS; ++i)
    {// assume that we have grouped all the spectra in the input workspace
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], std::sqrt(double(NHIST)), 0.0001 );
    }

    boost::shared_ptr<const IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<const DetectorGroup>(det) );
    TS_ASSERT_THROWS_ANYTHING( det = outputWS->getDetector(1) );

    AnalysisDataService::Instance().remove(output);
  }

  void testFileList()
  {
    // create a file in the current directory that we'll load later
    writeFileList();

    GroupDetectors2 grouper;
    grouper.initialize();
    grouper.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "File");
    grouper.setPropertyValue("OutputWorkspace", output);
    grouper.setPropertyValue("MapFile", inputFile);
    grouper.setProperty<bool>("KeepUngroupedSpectra",true);

    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), NHIST-1 );
    std::vector<double> tens(NBINS+1, 10.0);
    std::vector<double> ones(NBINS, 1.0);
    // check the two grouped spectra
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(NBINS, 1+3) ); // 1+3 = 4
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA(outputWS->dataE(0)[i], std::sqrt(static_cast<double>(2)), 1e-6);
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), 1 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(0)->getSpectrumNo(), 1);

    TS_ASSERT_EQUALS( outputWS->dataX(1), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(1), std::vector<double>(NBINS, 4 ) ); // Directly # 4
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 2 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(1)->getSpectrumNo(), 2);
    
    //check the unmoved spectra
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(2), std::vector<double>(NBINS, 2) );
    TS_ASSERT_EQUALS( outputWS->dataE(2), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), 2 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(2)->getSpectrumNo(), 2);


    TS_ASSERT_EQUALS( outputWS->dataX(3), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(3), std::vector<double>(NBINS, 5) );
    TS_ASSERT_EQUALS( outputWS->dataE(3), ones );

    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(3), 5 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(3)->getSpectrumNo(), 5);


    TS_ASSERT_EQUALS( outputWS->dataY(4), std::vector<double>(NBINS, 6) );
    TS_ASSERT_EQUALS( outputWS->dataE(4), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(4), 6 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(4)->getSpectrumNo(), 6);


    // the first two spectra should have a group of detectors the other spectra a single detector

    boost::shared_ptr<const IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<const DetectorGroup>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(1) );
    TS_ASSERT( boost::dynamic_pointer_cast<const Detector>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(2) );
    TS_ASSERT( boost::dynamic_pointer_cast<const Detector>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(3) );
    TS_ASSERT( boost::dynamic_pointer_cast<const Detector>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(4) );
    TS_ASSERT( boost::dynamic_pointer_cast<const Detector>(det) );
    
    AnalysisDataService::Instance().remove(output);
    remove(inputFile.c_str());
  }
  
  void testFileRanges()
  {
    // create a file in the current directory that we'll load later
    writeFileRanges();

    GroupDetectors2 grouper;
    grouper.initialize();
    grouper.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "File");
    grouper.setPropertyValue("OutputWorkspace", output);
    grouper.setPropertyValue("MapFile", inputFile);
    grouper.setProperty<bool>("KeepUngroupedSpectra",true);

    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), NHIST-3 );
    std::vector<double> tens(NBINS+1, 10.0);
    std::vector<double> ones(NBINS, 1.0);
    // check the first grouped spectrum
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(NBINS, 1+2+3) );
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA(outputWS->dataE(0)[i], std::sqrt(static_cast<double>(3)), 1e-6);
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), 1 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(0)->getSpectrumNo(), 1);

    // check the second grouped spectrum
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(1), std::vector<double>(NBINS, 4 ) );
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 2);
    TS_ASSERT_EQUALS( outputWS->getSpectrum(1)->getSpectrumNo(), 2);

    // check the third grouped spectrum
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(2), std::vector<double>(NBINS, 5+6 ) );
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA(
        outputWS->dataE(2)[i], std::sqrt(static_cast<double>(2)), 1e-6);
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), 3 );
    TS_ASSERT_EQUALS( outputWS->getSpectrum(2)->getSpectrumNo(), 3);

    AnalysisDataService::Instance().remove(output);
    remove(inputFile.c_str());
  }


  void testReadingFromXML()
  {
    Mantid::DataHandling::LoadMuonNexus1 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "MUSR00015190.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    

    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_1");
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 64);

    GroupDetectors2 groupAlg;
    groupAlg.initialize();
    groupAlg.setPropertyValue("InputWorkspace", outputSpace+"_1");
    groupAlg.setPropertyValue("OutputWorkspace", "boevs");
    groupAlg.setPropertyValue("MapFile", "IDFs_for_UNIT_TESTING/MUSR_Detector_Grouping.xml");
    TS_ASSERT_THROWS_NOTHING(groupAlg.execute());    
    TS_ASSERT( groupAlg.isExecuted() );

    MatrixWorkspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING(output1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("boevs"));    
    Workspace2D_sptr output2D1 = boost::dynamic_pointer_cast<Workspace2D>(output1);
    TS_ASSERT_EQUALS( output2D1->getNumberHistograms(), 2);

    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("boevs");
  }

  void testReadingFromXMLCheckDublicateIndex()
  {
    Mantid::DataHandling::LoadMuonNexus1 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "MUSR00015190.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    

    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_1");
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 64);

    GroupDetectors2 groupAlg;
    groupAlg.initialize();
    groupAlg.setPropertyValue("InputWorkspace", outputSpace+"_1");
    groupAlg.setPropertyValue("OutputWorkspace", "boevs");
    groupAlg.setPropertyValue("MapFile", "IDFs_for_UNIT_TESTING/MUSR_Detector_Grouping_dublicate.xml");
    TS_ASSERT_THROWS_NOTHING(groupAlg.execute());    
    TS_ASSERT( groupAlg.isExecuted() );

    MatrixWorkspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING(output1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("boevs"));    
    Workspace2D_sptr output2D1 = boost::dynamic_pointer_cast<Workspace2D>(output1);
    TS_ASSERT_EQUALS( output2D1->getNumberHistograms(), 2);

    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("boevs");
  }

  void testReadingFromXMLCheckDublicateIndex2()
  {
    Mantid::DataHandling::LoadMuonNexus1 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "MUSR00015190.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace="outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    

    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_1");
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 64);

    GroupDetectors2 groupAlg;
    groupAlg.initialize();
    groupAlg.setPropertyValue("InputWorkspace", outputSpace+"_1");
    groupAlg.setPropertyValue("OutputWorkspace", "boevs");
    groupAlg.setPropertyValue("MapFile", "IDFs_for_UNIT_TESTING/MUSR_Detector_Grouping_dublicate2.xml");
    TS_ASSERT_THROWS_NOTHING(groupAlg.execute());    
    TS_ASSERT( groupAlg.isExecuted() );

    MatrixWorkspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING(output1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("boevs"));    
    Workspace2D_sptr output2D1 = boost::dynamic_pointer_cast<Workspace2D>(output1);
    TS_ASSERT_EQUALS( output2D1->getNumberHistograms(), 4);

    std::set<detid_t>::const_iterator specDet;
    specDet = output2D1->getSpectrum(0)->getDetectorIDs().begin();
    TS_ASSERT_EQUALS( *specDet, 1);
    specDet = output2D1->getSpectrum(1)->getDetectorIDs().begin();
    TS_ASSERT_EQUALS( *specDet, 2);
    specDet = output2D1->getSpectrum(2)->getDetectorIDs().begin();
    TS_ASSERT_EQUALS( *specDet, 3); specDet++;
    TS_ASSERT_EQUALS( *specDet, 4); specDet++;
    TS_ASSERT_EQUALS( *specDet, 5);
    specDet = output2D1->getSpectrum(3)->getDetectorIDs().begin();
    TS_ASSERT_EQUALS( *specDet, 2); specDet++;
    TS_ASSERT_EQUALS( *specDet, 8); specDet++;
    TS_ASSERT_EQUALS( *specDet, 9); specDet++;
    TS_ASSERT_EQUALS( *specDet, 11); specDet++;
    TS_ASSERT_EQUALS( *specDet, 12); specDet++;
    TS_ASSERT_EQUALS( *specDet, 13);

    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("boevs");
  }

  void testAverageBehaviour()
  {
    Mantid::DataHandling::MaskDetectors mask;
    mask.initialize();
    mask.setPropertyValue("Workspace",inputWS);
    mask.setPropertyValue("WorkspaceIndexList","2");
    mask.execute();
    GroupDetectors2 gd2;
    gd2.initialize();
    gd2.setPropertyValue("InputWorkspace", inputWS);
    gd2.setPropertyValue("OutputWorkspace", "GroupDetectors2_testAverageBehaviour_Output");
    gd2.setPropertyValue("WorkspaceIndexList", "0-2");
    gd2.setPropertyValue("Behaviour", "Average");
    TS_ASSERT_THROWS_NOTHING(gd2.execute());

    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("GroupDetectors2_testAverageBehaviour_Output");

    // Result should be 1 + 2  / 2 = 1.5
    TS_ASSERT_EQUALS(output->readY(0)[1], 1.5);

    AnalysisDataService::Instance().remove("GroupDetectors2_testAverageBehaviour_Output");
  }

  void testEvents()
  {
      int numPixels = 5;
      int numBins = 5;
      int numEvents = 200;
      EventWorkspace_sptr input = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numBins, numEvents,0,1,4);
      AnalysisDataService::Instance().addOrReplace("GDEvents", input);
      GroupDetectors2 alg2;
      TS_ASSERT_THROWS_NOTHING( alg2.initialize());
      TS_ASSERT( alg2.isInitialized() );

      // Set the properties
      alg2.setPropertyValue("InputWorkspace","GDEvents");
      alg2.setPropertyValue("OutputWorkspace","GDEventsOut");
      alg2.setPropertyValue("WorkspaceIndexList", "2-4");
      alg2.setPropertyValue("Behaviour", "Average");
      alg2.setProperty("PreserveEvents", true);

      alg2.execute();
      TS_ASSERT(alg2.isExecuted());

      EventWorkspace_sptr output;
      output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("GDEventsOut");
      TS_ASSERT(output);
      TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(output->getNumberEvents(), (2+3+4)*numEvents);
      TS_ASSERT_EQUALS(input->readX(0).size(), output->readX(0).size());
      TS_ASSERT_DELTA((input->readY(2)[0]+input->readY(3)[0]+input->readY(4)[0])/3,output->readY(0)[0],0.00001);
      AnalysisDataService::Instance().remove("GDEventsOut");
  }

  
  void test_GroupingWorkspace_ThreeGroup_NoUngrouped_dontPreserveEvents_inplace()
  {
    dotestGroupingWorkspace(3, false, false,true,false);
  }

  void test_GroupingWorkspace_TwoGroup_Ungrouped_dontPreserveEvents_inplace()
  {
    dotestGroupingWorkspace(2, true, true,true,false);
  }

  void test_GroupingWorkspace_ThreeGroup_NoUngrouped_PreserveEvents_inplace()
  {
    dotestGroupingWorkspace(3, false, false,true,true);
  }

  void test_GroupingWorkspace_TwoGroup_Ungrouped_PreserveEvents_inplace()
  {
    dotestGroupingWorkspace(2, false, false,true,true);
  }

   void test_GroupingWorkspace_FourGroup_Ungrouped_PreserveEvents_Notinplace()
  {
    dotestGroupingWorkspace(4, true, true,true,false);
  }

  void dotestGroupingWorkspace(size_t numgroups=3, bool includeUngroupedDets =true, bool includeUngroupedDetsSetting=true, bool inplace = true,  bool preserveEvents = true, int bankWidthInPixels=8 )
  {
    std::string nxsWSname("GroupDetectors2TestTarget_ws");
    std::string groupWSName(nxsWSname + "_GROUP");
    std::string outputws = nxsWSname + "_grouped";

    // Create the fake event workspace
    EventWorkspace_sptr inputW = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(static_cast<int>(numgroups), bankWidthInPixels);
    AnalysisDataService::Instance().addOrReplace(nxsWSname, inputW);


    //-------- Check on the input workspace ---------------
    TS_ASSERT(inputW);
    if (!inputW) return;

    //Create an axis for each pixel. 
    for (size_t pix=0; pix < inputW->getNumberHistograms(); pix++)
    {
      cow_ptr<Mantid::MantidVec> axis;
      Mantid::MantidVec& xRef = axis.access();
      xRef.resize(5);
      for (int i = 0; i < 5; ++i)
        xRef[i] = static_cast<double>(1) + i*1.0;
      xRef[4] = 1e6;
      //Set an X-axis
      inputW->setX(pix, axis);
      inputW->getEventList(pix).addEventQuickly( TofEvent(1000.0) );
    }

    // ------------ Create a grouping workspace to match -------------
    auto groupW = boost::make_shared<GroupingWorkspace>(inputW->getInstrument());
    AnalysisDataService::Instance().addOrReplace(nxsWSname + "_GROUP", groupW);
    //fill in some groups
    size_t startingGroupNo= 1;
    size_t targetGroupNo = numgroups;
    size_t targetSpectraCount = numgroups;
    if (includeUngroupedDets)
    {
      --startingGroupNo;
      ++targetGroupNo;
    }
    size_t pixPerGroup = 0;
    if (numgroups > 0)
    {
      pixPerGroup = groupW->getNumberHistograms()/targetGroupNo;
    }
    if (includeUngroupedDets)
    {
      targetSpectraCount += includeUngroupedDetsSetting?pixPerGroup+1:0;
    }
    for (size_t pix=0; pix < groupW->getNumberHistograms(); pix++)
    {
      size_t groupNo = startingGroupNo + (pix/pixPerGroup);
      groupW->dataY(pix)[0] = static_cast<double>(groupNo);
    }

    // ------------ Create a grouping workspace by name -------------
    GroupDetectors2 groupAlg;
    groupAlg.initialize();
    TS_ASSERT_THROWS_NOTHING( groupAlg.setPropertyValue("InputWorkspace", nxsWSname) );
    if (inplace) outputws = nxsWSname;
    TS_ASSERT_THROWS_NOTHING( groupAlg.setPropertyValue("OutputWorkspace", outputws) );

    //This fake calibration file was generated using DiffractiongroupAlgsing2Test_helper.py
    TS_ASSERT_THROWS_NOTHING( groupAlg.setPropertyValue("CopyGroupingFromWorkspace", groupWSName) );
    
    TS_ASSERT_THROWS_NOTHING( groupAlg.setProperty("KeepUngroupedSpectra", includeUngroupedDetsSetting));
    TS_ASSERT_THROWS_NOTHING( groupAlg.setProperty("PreserveEvents", preserveEvents) );
    //OK, run the algorithm
    TS_ASSERT_THROWS_NOTHING( groupAlg.execute(); );
    TS_ASSERT( groupAlg.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(outputws) );
    if (!output) return;

    // ---- Did we keep the event workspace ----
    EventWorkspace_const_sptr outputEvent;
    TS_ASSERT_THROWS_NOTHING( outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(output) );
    if (preserveEvents)
    {
      TS_ASSERT( outputEvent );
      if (!outputEvent) return;
    }
    else
    {
      TS_ASSERT( !outputEvent );
    }


    TS_ASSERT_EQUALS( output->getNumberHistograms(), targetSpectraCount);
    
    AnalysisDataService::Instance().remove(nxsWSname);
    AnalysisDataService::Instance().remove(groupWSName);
    if (!inplace)  AnalysisDataService::Instance().remove(outputws);

  }


  void test_GroupingWorkspaceUsingMatrixWrokspace()
  {
    int bankWidth = 8;
    int numBanks = 2;
    int numSpectraInBank = bankWidth * bankWidth;
    int targetSpectraCount = 1 + (numBanks-1)*numSpectraInBank;
    std::string spectraToGroup = "0-"+boost::lexical_cast<std::string>((numSpectraInBank-1));
    
    std::string nxsWSname("GroupingWorkspaceUsingMatrixWrokspace_ws");
    std::string groupWSName(nxsWSname + "_GROUP");
    std::string outputws = nxsWSname + "_grouped";

    // Create the fake event workspace
    EventWorkspace_sptr inputW = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(numBanks, bankWidth);
    AnalysisDataService::Instance().addOrReplace(nxsWSname, inputW);
    
    //-------- Check on the input workspace ---------------
    TS_ASSERT(inputW);
    if (!inputW) return;

    //Create an axis for each pixel. 
    for (size_t pix=0; pix < inputW->getNumberHistograms(); pix++)
    {
      cow_ptr<Mantid::MantidVec> axis;
      Mantid::MantidVec& xRef = axis.access();
      xRef.resize(5);
      for (int i = 0; i < 5; ++i)
        xRef[i] = static_cast<double>(1) + i*1.0;
      xRef[4] = 1e6;
      //Set an X-axis
      inputW->setX(pix, axis);
      inputW->getEventList(pix).addEventQuickly( TofEvent(1000.0) );
    }

    // ------------ Create a grouped workspace using GroupDetectors -------------
    GroupDetectors2 groupAlg1;
    groupAlg1.initialize();
    TS_ASSERT_THROWS_NOTHING( groupAlg1.setPropertyValue("InputWorkspace", nxsWSname) );
    TS_ASSERT_THROWS_NOTHING( groupAlg1.setPropertyValue("OutputWorkspace", groupWSName) );

    //This fake calibration file was generated using DiffractiongroupAlg1sing2Test_helper.py
    TS_ASSERT_THROWS_NOTHING( groupAlg1.setProperty("WorkspaceIndexList", spectraToGroup) ); //group first bank
    
    TS_ASSERT_THROWS_NOTHING( groupAlg1.setProperty("KeepUngroupedSpectra", true));
    TS_ASSERT_THROWS_NOTHING( groupAlg1.setProperty("PreserveEvents", false) );
    //OK, run the algorithm
    TS_ASSERT_THROWS_NOTHING( groupAlg1.execute(); );
    TS_ASSERT( groupAlg1.isExecuted() );

    MatrixWorkspace_const_sptr outputGrp;
    TS_ASSERT_THROWS_NOTHING( outputGrp = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(groupWSName) );
    if (!outputGrp) return;

    TS_ASSERT_EQUALS( outputGrp->getNumberHistograms(), targetSpectraCount);

    // ------------ Create a grouping workspace by name -------------
    GroupDetectors2 groupAlg;
    groupAlg.initialize();
    TS_ASSERT_THROWS_NOTHING( groupAlg.setPropertyValue("InputWorkspace", nxsWSname) );
    TS_ASSERT_THROWS_NOTHING( groupAlg.setPropertyValue("OutputWorkspace", outputws) );

    //This fake calibration file was generated using DiffractiongroupAlgsing2Test_helper.py
    TS_ASSERT_THROWS_NOTHING( groupAlg.setPropertyValue("CopyGroupingFromWorkspace", groupWSName) );
    
    TS_ASSERT_THROWS_NOTHING( groupAlg.setProperty("KeepUngroupedSpectra", true));
    TS_ASSERT_THROWS_NOTHING( groupAlg.setProperty("PreserveEvents", false) );
    //OK, run the algorithm
    TS_ASSERT_THROWS_NOTHING( groupAlg.execute(); );
    TS_ASSERT( groupAlg.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(outputws) );
    if (!output) return;

    //check output - should match template
    TS_ASSERT_EQUALS( output->getNumberHistograms(), outputGrp->getNumberHistograms());
    TS_ASSERT_EQUALS( output->getDetector(0)->getID(), outputGrp->getDetector(0)->getID());
    
    
    AnalysisDataService::Instance().remove(nxsWSname);
    AnalysisDataService::Instance().remove(groupWSName);
    AnalysisDataService::Instance().remove(outputws);
 }

  private:
    const std::string inputWS, outputBase, inputFile;
    enum constants { NHIST = 6, NBINS = 4 };

    void writeFileList()
    {
      std::ofstream file(inputFile.c_str());
      file << " 2		#file format is in http://www.mantidproject.org/GroupDetectors \n"
        << "1 "            << std::endl // group id
        << "2"              << std::endl // number of spectra
        << "1   3"          << std::endl // the list of spectra

        << "  2"             << std::endl // group id
        << std::endl
        << "1"              << std::endl // 1 spectrum
        << "4"; // spectrum 4 is in the group
      file.close();
    }
    void writeFileRanges()
    {
      std::ofstream file(inputFile.c_str());
      file << "3		#file format is in http://www.mantidproject.org/GroupDetectors, using ranges \n"
        << "1 "            << std::endl
        << "3"              << std::endl
        << "  1-  3"          << std::endl
        << "2"             << std::endl
        << "1"              << std::endl
        << std::endl
        << "  4"              << std::endl
        << "3"              << std::endl
        << "2"              << std::endl
        << "5-6";
      file.close();
    }

};

#endif /*GROUPDETECTORS2TEST_H_*/
