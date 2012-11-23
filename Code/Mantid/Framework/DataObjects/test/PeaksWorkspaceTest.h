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
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/LogManager.h"

#include <Poco/File.h>


using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeaksWorkspaceTest : public CxxTest::TestSuite
{
public:

  PeaksWorkspaceTest()
  {
     FrameworkManager::Instance();
     AlgorithmManager::Instance();
  }

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

  void test_Save_Unmodified_PeaksWorkspace_Nexus()
  {
    const std::string inputWS("peaksWS_test_saveNexus");
    auto pw = createSaveTestPeaksWorkspace(inputWS);

    const V3D sampleFrameQ = pw->getPeak(0).getQSampleFrame();
    const V3D labFrameQ = pw->getPeak(0).getQLabFrame();

    const std::string filename = "test_Save_Unmodified_PeaksWorkspace_Nexus.nxs";
    auto lpw = saveAndReloadPeaksWorkspace(pw, filename);

    TS_ASSERT_EQUALS(17, lpw->columnCount());
    // Check that the peaks are what we saved
    TS_ASSERT_EQUALS( lpw->getPeak(0).getDetectorID(), 1300);
    TS_ASSERT_DELTA( lpw->getPeak(0).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( lpw->getPeak(0).getQSampleFrame(), sampleFrameQ);
    TS_ASSERT_EQUALS( lpw->getPeak(0).getQLabFrame(), labFrameQ);

    TS_ASSERT_EQUALS( lpw->getPeak(1).getDetectorID(), 1300);
    TS_ASSERT_DELTA(  lpw->getPeak(1).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_EQUALS( lpw->getPeak(2).getDetectorID(), 1350);
    TS_ASSERT_DELTA(  lpw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( lpw->getPeak(3).getDetectorID(), 1400);
    TS_ASSERT_DELTA( lpw->getPeak(3).getWavelength(), 3.0, 1e-5);
  }


  void test_getSetLogAccess()
  {
    bool trueSwitch(true);
    PeaksWorkspace * pw = buildPW();

    LogManager_const_sptr props = pw->getLogs();
    std::string existingVal;

    TS_ASSERT_THROWS_NOTHING(existingVal=props->getPropertyValueAsType<std::string>("TestProp"));
    TS_ASSERT_EQUALS("value",existingVal);

    // define local scope;
    if(trueSwitch)
    {
      // get mutable pointer to existing values;
      LogManager_sptr mprops = pw->logs();

      TS_ASSERT_THROWS_NOTHING(mprops->addProperty<std::string>("TestProp2","value2"));

      TS_ASSERT(mprops->hasProperty("TestProp2"));
      TS_ASSERT(!props->hasProperty("TestProp2"));
      TS_ASSERT(pw->run().hasProperty("TestProp2"));
    }
    // nothing terrible happened and workspace still have this property
    TS_ASSERT(pw->run().hasProperty("TestProp2"));

    auto pw1 = pw->clone();
    if(trueSwitch)
    {
      // get mutable pointer to existing values, which would be taken from the cash
      LogManager_sptr mprops1 = pw->logs();
      // and in ideal world this should cause CowPtr to diverge but it does not
      TS_ASSERT_THROWS_NOTHING(mprops1->addProperty<std::string>("TestProp1-3","value1-3"));
      TS_ASSERT(mprops1->hasProperty("TestProp1-3"));
      //THE CHANGES TO PW ARE APPLIED TO the COPY (PW1 too!!!!)
      TS_ASSERT(pw->run().hasProperty("TestProp1-3"));
      TS_ASSERT(pw1->run().hasProperty("TestProp1-3"));
    }
    TS_ASSERT(pw1->run().hasProperty("TestProp1-3"));
    if(trueSwitch)
    {
      // but this will cause it to diverge
      LogManager_sptr mprops2 = pw1->logs();
      // and this  causes CowPtr to diverge
      TS_ASSERT_THROWS_NOTHING(mprops2->addProperty<std::string>("TestProp2-3","value2-3"));
      TS_ASSERT(mprops2->hasProperty("TestProp2-3"));
      TS_ASSERT(!pw->run().hasProperty("TestProp2-3"));
      TS_ASSERT(pw1->run().hasProperty("TestProp2-3"));
    }


    TSM_ASSERT_THROWS_NOTHING("should clearly delete pw1",pw1.reset());

    TSM_ASSERT_THROWS_NOTHING("should clearly delete pw",delete pw);
  }

   void test_hasIntegratedPeaks_without_property()
   {
     PeaksWorkspace ws;
     TSM_ASSERT("Should not indicate that there are integrated peaks without property.", !ws.hasIntegratedPeaks());
   }

   void test_hasIntegratedPeaks_with_property_when_false()
   {
     PeaksWorkspace ws;
     bool hasIntegratedPeaks = false;
     ws.mutableRun().addProperty("PeaksIntegrated", hasIntegratedPeaks);
     TS_ASSERT_EQUALS(hasIntegratedPeaks, ws.hasIntegratedPeaks());
   }

   void test_hasIntegratedPeaks_with_property_when_true()
   {
     PeaksWorkspace ws;
     bool hasIntegratedPeaks = true;
     ws.mutableRun().addProperty("PeaksIntegrated", hasIntegratedPeaks);
     TS_ASSERT_EQUALS(hasIntegratedPeaks, ws.hasIntegratedPeaks());
   }

private:

   PeaksWorkspace_sptr createSaveTestPeaksWorkspace(const std::string & adsName)
   {
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
     auto pw = boost::make_shared<PeaksWorkspace>();
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

     AnalysisDataService::Instance().add(adsName, pw);
     return pw;
   }

   PeaksWorkspace_sptr saveAndReloadPeaksWorkspace(const PeaksWorkspace_sptr & pws, const std::string & filename)
   {

     IAlgorithm_sptr saver = AlgorithmManager::Instance().createUnmanaged("SaveNexus");
     saver->setChild(true);
     saver->initialize();
     saver->setProperty<Workspace_sptr>("InputWorkspace", pws);
     saver->setPropertyValue("Filename", filename);
     TS_ASSERT_THROWS_NOTHING(saver->execute());
     TS_ASSERT(saver->isExecuted());

     // Load the nexus file
     IAlgorithm_sptr loader = AlgorithmManager::Instance().createUnmanaged("LoadNexus");
     loader->setChild(true);
     loader->initialize();
     const std::string absFilename = saver->getPropertyValue("Filename");
     loader->setPropertyValue("Filename", absFilename); // absolute path
     loader->setPropertyValue("OutputWorkspace", "__anonymous_output");
     TS_ASSERT_THROWS_NOTHING(loader->execute());
     TS_ASSERT(loader->isExecuted());

     // Remove file
     if (Poco::File(absFilename).exists())
       Poco::File(absFilename).remove();

     Workspace_sptr ws = loader->getProperty("OutputWorkspace");
     PeaksWorkspace_sptr lpw = boost::dynamic_pointer_cast<PeaksWorkspace>(ws);
     TS_ASSERT(lpw);
     return lpw;
   }

};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

