#ifndef GROUPDETECTORS2TEST_H_
#define GROUPDETECTORS2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidNexus/LoadMuonNexus2.h"
#include <iostream>
#include <numeric>
#include <fstream>
#include <Poco/Path.h>

using Mantid::DataHandling::GroupDetectors2;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::NeXus;

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
    // This is needed to load in the plugin algorithms (specifically Divide, which is a sub-algorithm of GroupDetectors)
    FrameworkManager::Instance();
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space =
      WorkspaceFactory::Instance().create("Workspace2D", NHIST, NBINS+1, NBINS);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Mantid::MantidVecPtr xs, errors, data[NHIST];
    xs.access().resize(NBINS+1, 10.0);
    errors.access().resize(NBINS, 1.0);
    int detIDs[NHIST];
    int specNums[NHIST];
    for (int j = 0; j < NHIST; ++j)
    {
      space2D->setX(j,xs);
      data[j].access().resize(NBINS, j + 1);  // the y values will be different for each spectra (1+index_number) but the same for each bin
      space2D->setData(j, data[j], errors);
      space2D->getAxis(1)->spectraNo(j) = j+1;  // spectra numbers are also 1 + index_numbers because this is the tradition
      detIDs[j] = j;
      specNums[j] = j+1;
    }

    Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(space->getBaseInstrument());

    Detector *d = new Detector("det",0,0);
    instr->markAsDetector(d);
    Detector *d1 = new Detector("det",1,0);
    instr->markAsDetector(d1);
    Detector *d2 = new Detector("det",2,0);
    instr->markAsDetector(d2);
    Detector *d3 = new Detector("det",3,0);
    instr->markAsDetector(d3);
    Detector *d4 = new Detector("det",4,0);
    instr->markAsDetector(d4);
    Detector *d5 = new Detector("det",5,0);
    instr->markAsDetector(d5);

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space->mutableSpectraMap().populate(specNums, detIDs, NHIST );

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
    TS_ASSERT_EQUALS( gd.category(), "DataHandling\\Detectors" );
    TS_ASSERT_THROWS_NOTHING( gd.initialize() );
    TS_ASSERT( gd.isInitialized() );

    gd.setPropertyValue("InputWorkspace", inputWS);
    gd.setPropertyValue("OutputWorkspace", outputBase);
    TS_ASSERT_THROWS_NOTHING( gd.execute());
    TS_ASSERT( ! gd.isExecuted() );
    
    AnalysisDataService::Instance().remove(outputBase);
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

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) );
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

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) );
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

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) );
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
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(NBINS, 1+3) );
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA(outputWS->dataE(0)[i], std::sqrt(static_cast<double>(2)), 1e-6);
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), 1 );
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(1), std::vector<double>(NBINS, 4 ) );
    
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones );

      TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 4 );
    //check the unmoved spectra
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(2), std::vector<double>(NBINS, 2) );
    TS_ASSERT_EQUALS( outputWS->dataE(2), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), 2 );
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(3), std::vector<double>(NBINS, 5) );
    TS_ASSERT_EQUALS( outputWS->dataE(3), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(3), 5 );
    TS_ASSERT_EQUALS( outputWS->dataY(4), std::vector<double>(NBINS, 6) );
    TS_ASSERT_EQUALS( outputWS->dataE(4), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(4), 6 );

    // the first two spectra should have a group of detectors the other spectra a single detector

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) );
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(1) );
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(2) );
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(3) );
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) );
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(4) );
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) );
    
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
    // check the second grouped spectrum
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(1), std::vector<double>(NBINS, 4 ) );
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones );
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 4 );
    // check the third grouped spectrum
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens );
    TS_ASSERT_EQUALS( outputWS->dataY(2), std::vector<double>(NBINS, 5+6 ) );
    for (int i = 0; i < NBINS; ++i)
    {
      TS_ASSERT_DELTA(
        outputWS->dataE(2)[i], std::sqrt(static_cast<double>(2)), 1e-6);
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), 5 );

    AnalysisDataService::Instance().remove(output);
    remove(inputFile.c_str());
  }


  void testReadingFromXML()
  {
#ifndef _WIN64
    LoadMuonNexus2 nxLoad;
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
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_1"));
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
    TS_ASSERT_THROWS_NOTHING(output1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("boevs")));    
    Workspace2D_sptr output2D1 = boost::dynamic_pointer_cast<Workspace2D>(output1);
    TS_ASSERT_EQUALS( output2D1->getNumberHistograms(), 2);

    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("boevs");
#endif
  }

  void testAverageBehaviour()
  {
    GroupDetectors2 gd2;
    gd2.initialize();
    gd2.setPropertyValue("InputWorkspace", inputWS);
    gd2.setPropertyValue("OutputWorkspace", "GroupDetectors2_testAverageBehaviour_Output");
    gd2.setPropertyValue("WorkspaceIndexList", "0-2");
    gd2.setPropertyValue("Behaviour", "Average");
    TS_ASSERT_THROWS_NOTHING(gd2.execute());

    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("GroupDetectors2_testAverageBehaviour_Output"));

    // Result should be 1 + 2 + 3 / 3 = 2
    TS_ASSERT_EQUALS(output->readY(0)[1], 2.0);

    AnalysisDataService::Instance().remove("GroupDetectors2_testAverageBehaviour_Output");
  }

  private:
    const std::string inputWS, outputBase, inputFile;
    enum constants { NHIST = 6, NBINS = 4 };

    void writeFileList()
    {
      std::ofstream file(inputFile.c_str());
      file << " 2		#file format is in http://svn.mantidproject.org/mantid/trunk/Code/Mantid/Framework/DataHandling/inc/MantidDataHandling/GroupDetectors2.h \n"
        << "888 "            << std::endl
        << "2"              << std::endl
        << "1   3"          << std::endl
        << "  888"             << std::endl
        << std::endl
        << "1"              << std::endl
        << "4";
      file.close();
    }
    void writeFileRanges()
    {
      std::ofstream file(inputFile.c_str());
      file << "3		#file format is in http://svn.mantidproject.org/mantid/trunk/mantid/Code/Mantid/DataHandling/inc/MantidDataHandling/GroupDetectors3.h \n"
        << "0 "            << std::endl
        << "3"              << std::endl
        << "  1-  3"          << std::endl
        << "0"             << std::endl
        << "1"              << std::endl
        << std::endl
        << "  4"              << std::endl
        << "0"              << std::endl
        << "2"              << std::endl
        << "5-6";
      file.close();
    }

};

#endif /*GROUPDETECTORS2TEST_H_*/
