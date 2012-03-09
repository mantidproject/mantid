#ifndef MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cmath>
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include <Poco/File.h>


using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeaksWorkspaceTest : public CxxTest::TestSuite
{
public:

  /** Build a test PeaksWorkspace
   *
   * @return PeaksWorkspace
   */
  PeaksWorkspace * buildPW()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    PeaksWorkspace * pw = new PeaksWorkspace();
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return pw;
  }

  /** Check that the PeaksWorkspace build by buildPW() is correct */
  void checkPW(PeaksWorkspace * pw)
  {
    TS_ASSERT_EQUALS( pw->columnCount(), 17);
    TS_ASSERT_EQUALS( pw->rowCount(), 1);
    TS_ASSERT_EQUALS( pw->getNumberPeaks(), 1);
    if (pw->getNumberPeaks() != 1) return;
    TS_ASSERT_DELTA( pw->getPeak(0).getWavelength(), 3.0, 1e-4);
    // Experiment info stuff got copied
    TS_ASSERT_EQUALS( pw->getInstrument()->getName(), "SillyInstrument");
    TS_ASSERT( pw->run().hasProperty("TestProp") );
  }

  void test_defaultConstructor()
  {
    PeaksWorkspace * pw = buildPW();
    checkPW(pw);
    delete pw;
  }

  void test_copyConstructor()
  {
    PeaksWorkspace * pw = buildPW();
    PeaksWorkspace * pw2 = new PeaksWorkspace(*pw);
    checkPW(pw2);
    delete pw;
    delete pw2;
  }

  void test_clone()
  {
    PeaksWorkspace_sptr pw(buildPW());
    PeaksWorkspace_sptr pw2 = pw->clone();
    checkPW(pw2.get());
  }

  void test_sort()
  {
    PeaksWorkspace_sptr pw(buildPW());
    Instrument_const_sptr inst = pw->getInstrument();
    Peak p0 = pw->getPeak(0); //Peak(inst, 1, 3.0)
    Peak p1(inst, 1, 4.0);
    Peak p2(inst, 1, 5.0);
    Peak p3(inst, 2, 3.0);
    Peak p4(inst, 3, 3.0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back( std::pair<std::string, bool>("detid", true) );
    criteria.push_back( std::pair<std::string, bool>("wavelength", false) );
    pw->sort(criteria);
    TS_ASSERT_EQUALS( pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(0).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(1).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(1).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(2).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(3).getDetectorID(), 2);
    TS_ASSERT_DELTA(  pw->getPeak(3).getWavelength(), 3.0, 1e-5);

    // Sort by wavelength ascending then detID
    criteria.clear();
    criteria.push_back( std::pair<std::string, bool>("wavelength", true) );
    criteria.push_back( std::pair<std::string, bool>("detid", true) );
    pw->sort(criteria);
    TS_ASSERT_EQUALS( pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(0).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(1).getDetectorID(), 2);
    TS_ASSERT_DELTA(  pw->getPeak(1).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(2).getDetectorID(), 3);
    TS_ASSERT_DELTA(  pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(3).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(3).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(4).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(4).getWavelength(), 5.0, 1e-5);

  }

  void test_saveNexus()
  {
    // Ensure the plugin libraries are loaded so that we can use LoadNexusProcessed
    Mantid::API::FrameworkManager::Instance();

    // get an instrument which we load into a dummy workspace and get it from that workspace
    const std::string inst_filename = "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING5.xml"; 
    IAlgorithm_sptr inst_loader = AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
    inst_loader->initialize(); 
    inst_loader->setPropertyValue("Filename", inst_filename);
    std::string inst_output_ws("DummyWorkspaceToGetIDF");
    inst_loader->setPropertyValue("OutputWorkspace", inst_output_ws);
    TS_ASSERT_THROWS_NOTHING(inst_loader->execute());

    MatrixWorkspace_sptr dummyWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inst_output_ws);
    Instrument_const_sptr inst = dummyWS->getInstrument();


    // Create peak workspace
    PeaksWorkspace_sptr pw(new PeaksWorkspace()); //(buildPW());

    // Populate peak workspace with instrument from dummy workspace
    pw->setInstrument(inst);

    // Populate peak workspace with peaks
    Peak p1(inst, 1300, 4.0);
    Peak p2(inst, 1300, 5.0);
    Peak p3(inst, 1350, 3.0);
    Peak p4(inst, 1400, 3.0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);


     // Save it to Nexus
    std::string input_ws("peaksWS_test_saveNexus");

    AnalysisDataService::Instance().add( input_ws, pw );

    const std::string filename = "PeaksWorksapceTest_test_saveNexus.nxs";
    IAlgorithm_sptr saver = AlgorithmManager::Instance().createUnmanaged("SaveNexusProcessed");
    saver->initialize();

    saver->setPropertyValue("InputWorkspace", input_ws);
    saver->setPropertyValue("Filename", filename);
    TS_ASSERT_THROWS_NOTHING(saver->execute());


    // Load the nexus file
    IAlgorithm_sptr loader = AlgorithmManager::Instance().createUnmanaged("LoadNexusProcessed"); 
    loader->initialize();

    std::string output_ws("loaded_peaks");

    loader->setPropertyValue("Filename", filename);
    loader->setPropertyValue("OutputWorkspace", output_ws);
    TS_ASSERT_THROWS_NOTHING(loader->execute());

    //Test some aspects of the peaks loaded from the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );

    PeaksWorkspace_sptr lpw = boost::dynamic_pointer_cast<PeaksWorkspace>(workspace);

    // Check that the peaks are what we saved
    TS_ASSERT_EQUALS( lpw->getPeak(0).getDetectorID(), 1300);
    TS_ASSERT_DELTA(  lpw->getPeak(0).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( lpw->getPeak(1).getDetectorID(), 1300);
    TS_ASSERT_DELTA(  lpw->getPeak(1).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_EQUALS( lpw->getPeak(2).getDetectorID(), 1350);
    TS_ASSERT_DELTA(  lpw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( lpw->getPeak(3).getDetectorID(), 1400);
    TS_ASSERT_DELTA(  lpw->getPeak(3).getWavelength(), 3.0, 1e-5);

    //Remove file
    remove( filename.c_str() );

  }


};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

