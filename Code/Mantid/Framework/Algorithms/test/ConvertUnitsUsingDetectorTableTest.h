#ifndef CONVERTUNITSUSINGDETECTORTABLETEST_H_
#define CONVERTUNITSUSINGDETECTORTABLETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class ConvertUnitsUsingDetectorTableTest : public CxxTest::TestSuite
{
public:
  void setup_WS()
  {
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",256,11,10);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    boost::shared_ptr<Mantid::MantidVec> x(new Mantid::MantidVec(11));
    for (int i = 0; i < 11; ++i)
    {
      (*x)[i]=i*1000;
    }
    boost::shared_ptr<Mantid::MantidVec> a(new Mantid::MantidVec(10));
    boost::shared_ptr<Mantid::MantidVec> e(new Mantid::MantidVec(10));
    for (int i = 0; i < 10; ++i)
    {
      (*a)[i]=i;
      (*e)[i]=sqrt(double(i));
    }
    for (int j = 0; j < 256; ++j) {
      space2D->setX(j, x);
      space2D->setData(j, a, e);
      // Just set the spectrum number to match the index
      space2D->getSpectrum(j)->setSpectrumNo(j);
      space2D->getSpectrum(j)->setDetectorID(j);
    }
    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    // Register the workspace in the data service
    this->inputSpace = "testWorkspace";
    AnalysisDataService::Instance().addOrReplace(inputSpace, space);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    const std::string inputFile = ConfigService::Instance().getInstrumentDirectory() + "HET_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", this->inputSpace);
    loader.setProperty("RewriteSpectraMap",false);
    loader.execute();
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  /* Test that when the units are the same between the input workspace and the target, AND the output workspace name IS the same as the input workspace name,
   * that the input workspace and output workspace point to the same in-memory workspace.
   */
  void test_Exec_Input_Same_Output_And_Same_Units()
  {
    this->setup_WS();
    if (!alg.isInitialized())
      alg.initialize();

    auto inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputSpace);
    // Set the properties
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", inputSpace);
    alg.setPropertyValue("OutputWorkspace", inputSpace); // OutputWorkspace == InputWorkspace
    alg.setPropertyValue("Target", "TOF"); // Same as the input workspace.
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputSpace);
    TSM_ASSERT_EQUALS("Input and Output Workspaces should be pointer identical.", inWS.get(), outWS.get());
    AnalysisDataService::Instance().remove(inputSpace);
  }

  /* Test that when the units are the same between the input workspace and the target, AND the output workspace name IS NOT the same as the input workspace name,
   * that the input workspace and output workspace do not point to the same in-memory workspace.
   */
  void test_Exec_Input_different_Output_But_Same_Units()
  {
    this->setup_WS();
    if (!alg.isInitialized())
      alg.initialize();

    auto inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputSpace);
    // Set the properties
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", inputSpace);
    const std::string outputWorkspaceName = "OutWSName";
    alg.setPropertyValue("OutputWorkspace", outputWorkspaceName); // OutputWorkspace == InputWorkspace
    alg.setPropertyValue("Target", "TOF"); // Same as the input workspace.
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspaceName);
    TSM_ASSERT_DIFFERS("Input and Output Workspaces be completely different objects.", inWS.get(), outWS.get());
    AnalysisDataService::Instance().remove(outputWorkspaceName);
    AnalysisDataService::Instance().remove(inputSpace);
  }

  void testExec()
  {
    this->setup_WS();
    if ( !alg.isInitialized() ) alg.initialize();

    // Set the properties
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace",inputSpace);
    outputSpace = "outWorkspace";
    alg.setPropertyValue("OutputWorkspace",outputSpace);
    alg.setPropertyValue("Target","Wavelength");
    alg.setPropertyValue("AlignBins","1");

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    alg.isExecuted();

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
    // Check that the output unit is correct
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "Wavelength");
    // Test that y & e data is unchanged
    Mantid::MantidVec y = output2D->dataY(101);
    Mantid::MantidVec e = output2D->dataE(101);
    unsigned int ten = 10;
    TS_ASSERT_EQUALS( y.size(), ten );
    TS_ASSERT_EQUALS( e.size(), ten );
    Mantid::MantidVec yIn = input2D->dataY(101);
    Mantid::MantidVec eIn = input2D->dataE(101);
    TS_ASSERT_DELTA( y[0], yIn[0], 1e-6 );
    TS_ASSERT_DELTA( y[4], yIn[4], 1e-6 );
    TS_ASSERT_DELTA( e[1], eIn[1], 1e-6 );
    // Test that spectra that should have been zeroed have been
    Mantid::MantidVec x = output2D->dataX(0);
    y = output2D->dataY(0);
    e = output2D->dataE(0);
    TS_ASSERT_EQUALS( y[1], 0 );
    TS_ASSERT_EQUALS( e[9], 0 );
    // Check that the data has truly been copied (i.e. isn't a reference to the same
    //    vector in both workspaces)
    double test[10] = {11, 22, 33, 44, 55, 66, 77, 88, 99, 1010};
    boost::shared_ptr<Mantid::MantidVec > tester(new Mantid::MantidVec(test, test+10));
    output2D->setData(111, tester, tester);
    y = output2D->dataY(111);
    TS_ASSERT_EQUALS( y[3], 44.0);
    yIn = input2D->dataY(111);
    TS_ASSERT_EQUALS( yIn[3], 3.0);

    // Check that a couple of x bin boundaries have been correctly converted
    x = output2D->dataX(103);
    TS_ASSERT_DELTA( x[5], 1.5808, 0.0001 );
    TS_ASSERT_DELTA( x[10], 3.1617, 0.0001 );
    // Just check that an input bin boundary is unchanged
    Mantid::MantidVec xIn = input2D->dataX(66);
    TS_ASSERT_EQUALS( xIn[4], 4000.0 );

    AnalysisDataService::Instance().remove("outputSpace");
  }

  void testConvertUsingDetectorTable()
  {
     ConvertUnitsUsingDetectorTable myAlg;
     myAlg.initialize();
     TS_ASSERT(myAlg.isInitialized());

     const std::string workspaceName("_ws_testConvertUsingDetectorTable");
     int nBins = 1000;
     MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(2, nBins, 5.0, 50.0);
     WS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      
     AnalysisDataService::Instance().add(workspaceName,WS);

      // Create TableWorkspace with values in it

      ITableWorkspace_sptr pars = WorkspaceFactory::Instance().createTable("TableWorkspace");
      pars->addColumn("int", "spectra");
      pars->addColumn("double", "l1");
      pars->addColumn("double", "l2");
      pars->addColumn("double", "twotheta");
      pars->addColumn("double", "efixed");
      pars->addColumn("int", "emode");

      API::TableRow row0 = pars->appendRow();
      row0 << 1 << 50.0 << 10.0 << M_PI/2.0 << 7.0 << 1;

      API::TableRow row1 = pars->appendRow();
      row1 << 2 << 100.0 << 10.0 << 90.0 << 7.0 << 1;

      // Set the properties
      myAlg.setRethrows(true);
      myAlg.setPropertyValue("InputWorkspace", workspaceName);
      myAlg.setPropertyValue("OutputWorkspace", workspaceName);
      myAlg.setPropertyValue("Target", "Energy");
      myAlg.setProperty("DetectorParameters", pars);

      myAlg.execute();

      auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);

//      for (int j=0; j < outWS->getNumberHistograms(); ++j) {
//          for (int i=0; i < outWS->blocksize(); ++i) {
//              std::cout << "dataX[" << j << "]["<< i << "] = " << outWS->dataX(j)[i] << std::endl;
//          }
//      }
      
      TS_ASSERT_DELTA( outWS->dataX(1)[1], 25.3444, 0.01 );
      // TODO: Add more checks.
      
      AnalysisDataService::Instance().remove(workspaceName);
  }

  void testConvertQuickly()
  {
    ConvertUnitsUsingDetectorTable quickly;
    quickly.initialize();
    TS_ASSERT( quickly.isInitialized() );
    quickly.setPropertyValue("InputWorkspace",outputSpace);
    quickly.setPropertyValue("OutputWorkspace","quickOut2");
    quickly.setPropertyValue("Target","Energy");
    TS_ASSERT_THROWS_NOTHING( quickly.execute() );
    TS_ASSERT( quickly.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("quickOut2") );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "Energy");
    TS_ASSERT_DELTA( output->dataX(1)[1], 10.10, 0.01 );

    AnalysisDataService::Instance().remove("quickOut2");
  }

  void testConvertQuicklyCommonBins()
  {
    Workspace2D_sptr input = WorkspaceCreationHelper::Create2DWorkspace123(3,10,1);
    input->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    AnalysisDataService::Instance().add("quickIn", input);
    ConvertUnitsUsingDetectorTable quickly;
    quickly.initialize();
    TS_ASSERT( quickly.isInitialized() );
    quickly.setPropertyValue("InputWorkspace","quickIn");
    quickly.setPropertyValue("OutputWorkspace","quickOut");
    quickly.setPropertyValue("Target","dSpacing");
    TS_ASSERT_THROWS_NOTHING( quickly.execute() );
    TS_ASSERT( quickly.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("quickOut") );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS( &(output->dataX(0)[0]), &(output->dataX(0)[0]) );
    const size_t xsize = output->blocksize();
    for(size_t i = 0; i < output->getNumberHistograms(); ++i)
    {
      const auto & outX = output->readX(i);
      for(size_t j = 0; j <= xsize; ++j)
      {
        TS_ASSERT_EQUALS( outX[j], 2.0*M_PI );
      }
    }

    AnalysisDataService::Instance().remove("quickIn");
    AnalysisDataService::Instance().remove("quickOut");
  }

  void testDeltaE()
  {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,2663,5,7.5);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    Instrument_sptr testInst(new Instrument);
    ws->setInstrument(testInst);
    // Make it look like MARI (though not bin boundaries are different to the real MARI file used before)
    // Define a source and sample position
    //Define a source component
    ObjComponent *source = new ObjComponent("moderator", Object_sptr(), testInst.get());
    source->setPos(V3D(0, 0.0, -11.739));
    testInst->add(source);
    testInst->markAsSource(source);
    // Define a sample as a simple sphere
    ObjComponent *sample = new ObjComponent("samplePos", Object_sptr(), testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);
    Detector * physicalPixel = new Detector("pixel", 1, testInst.get());
    physicalPixel->setPos(-0.34732,-3.28797,-2.29022);
    testInst->add(physicalPixel);
    testInst->markAsDetector(physicalPixel);
    ws->getSpectrum(0)->addDetectorID(physicalPixel->getID());

    ConvertUnitsUsingDetectorTable conv;
    conv.initialize();
    conv.setProperty("InputWorkspace",ws);
    std::string outputSpace = "outWorkspace";
    conv.setPropertyValue("OutputWorkspace",outputSpace);
    conv.setPropertyValue("Target","DeltaE");
    conv.setPropertyValue("Emode","Direct");
    conv.setPropertyValue("Efixed","12.95");
    conv.execute();

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace) );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_EQUALS( output->blocksize(), 1669 );

    ConvertUnitsUsingDetectorTable conv2;
    conv2.initialize();
    conv2.setProperty("InputWorkspace",ws);
    conv2.setPropertyValue("OutputWorkspace",outputSpace);
    conv2.setPropertyValue("Target","DeltaE_inWavenumber");
    conv2.setPropertyValue("Emode","Indirect");
    conv2.setPropertyValue("Efixed","10");
    conv2.execute();

    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace) );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "DeltaE_inWavenumber");
    TS_ASSERT_EQUALS( output->blocksize(), 2275 );

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void setup_Event()
  {
    this->inputSpace = "eventWS";
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10,false);
    AnalysisDataService::Instance().addOrReplace(inputSpace, ws);
  }

  void testExecEvent_sameOutputWS()
  {
    std::size_t wkspIndex = 0;
    this->setup_Event();

    //Retrieve Workspace
    EventWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputSpace);
    TS_ASSERT( WS ); //workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    EventList el = WS->getEventList(wkspIndex);
    double a_tof = el.getEvents()[0].tof();
    double a_x = el.dataX()[1];

    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT( alg.isInitialized() );

    //Set all the properties
    alg.setPropertyValue("InputWorkspace", inputSpace);
    alg.setPropertyValue("Target", "DeltaE");
    alg.setPropertyValue("EMode", "Direct");
    alg.setPropertyValue("Efixed", "15.0");
    this->outputSpace = inputSpace;
    alg.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    //Things that haven't changed
    TS_ASSERT_EQUALS( start_blocksize, WS->blocksize());
    TS_ASSERT_EQUALS( num_events, WS->getNumberEvents() );
    //But a TOF changed.
    TS_ASSERT_DIFFERS(a_tof, WS->getEventList(wkspIndex).getEvents()[0].tof());
    //and a X changed
    TS_ASSERT_DIFFERS(a_x, WS->getEventList(wkspIndex).dataX()[1]);
  }

  void testExecEvent_TwoStepConversionWithDeltaE()
  {
    // Test to make sure the TOF->DeltaE->Other Quantity works for
    // EventWorkspaces
    this->setup_Event();

    ConvertUnitsUsingDetectorTable conv;
    conv.initialize();
    conv.setPropertyValue("InputWorkspace", this->inputSpace);
    conv.setPropertyValue("OutputWorkspace", this->inputSpace);
    conv.setPropertyValue("Target","DeltaE");
    conv.setPropertyValue("Emode","Direct");
    conv.setPropertyValue("Efixed","15.0");
    conv.execute();

    ConvertUnitsUsingDetectorTable conv2;
    conv2.initialize();
    conv2.setPropertyValue("InputWorkspace", this->inputSpace);
    conv2.setPropertyValue("OutputWorkspace", this->inputSpace);
    conv2.setPropertyValue("Target","Wavelength");
    conv2.setPropertyValue("Emode","Direct");
    conv2.setPropertyValue("Efixed","15.0");
    TS_ASSERT_THROWS_NOTHING( conv2.execute() );
    TS_ASSERT( conv2.isExecuted() );
  }


  /** Ticket #3934: If the workspace is sorted by TOF, it should remain so even if
   * sorting flips the direction
   */
  void do_testExecEvent_RemainsSorted(EventSortType sortType, std::string targetUnit)
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10, false);
    ws->getAxis(0)->setUnit("TOF");
    ws->sortAll(sortType, NULL);

    if (sortType == TOF_SORT)
    {
      // Only threadsafe if all the event lists are sorted
      TS_ASSERT( ws->threadSafe() );
    }
    TS_ASSERT_EQUALS( ws->getNumberEvents(), 100*200);

    ConvertUnitsUsingDetectorTable conv;
    conv.initialize();
    conv.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws));
    conv.setPropertyValue("OutputWorkspace", "out");
    conv.setPropertyValue("Target",targetUnit);
    conv.execute();
    TS_ASSERT( conv.isExecuted() );

    EventWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("out");
    TS_ASSERT(out);
    if (!out) return;
    TS_ASSERT_EQUALS( out->getNumberEvents(), 100*200);

    EventList & el = out->getEventList(0);
    TS_ASSERT( el.getSortType() == sortType );

    if (sortType == TOF_SORT)
    {
      // Only threadsafe if all the event lists are sorted by TOF
      TS_ASSERT( out->threadSafe() );

      // Check directly that it is indeed increasing
      double last_x = -1e10;
      for (size_t i=0; i<el.getNumberEvents(); i++)
      {
        double x = el.getEvent(i).tof();
        TS_ASSERT( x >= last_x );
        last_x = x;
      }
    }
    else if (sortType == PULSETIME_SORT)
    {
      // Check directly that it is indeed increasing
      Mantid::Kernel::DateAndTime last_x;
      for (size_t i=0; i<el.getNumberEvents(); i++)
      {
        Mantid::Kernel::DateAndTime x = el.getEvent(i).pulseTime();
        TS_ASSERT( x >= last_x );
        last_x = x;
      }
    }
  }

  void testExecEvent_RemainsSorted_TOF()
  {
    do_testExecEvent_RemainsSorted(TOF_SORT, "dSpacing");
  }

  void testExecEvent_RemainsSorted_Pulsetime()
  {
    do_testExecEvent_RemainsSorted(PULSETIME_SORT, "dSpacing");
  }

  void testExecEvent_RemainsSorted_TOF_to_Energy()
  {
    do_testExecEvent_RemainsSorted(TOF_SORT, "Energy");
  }

  void testExecEvent_RemainsSorted_Pulsetime_to_Energy()
  {
    do_testExecEvent_RemainsSorted(PULSETIME_SORT, "Energy");
  }

private:
  ConvertUnitsUsingDetectorTable alg;
  std::string inputSpace;
  std::string outputSpace;
};

class ConvertUnitsUsingDetectorTableTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertUnitsUsingDetectorTableTestPerformance *createSuite() { return new ConvertUnitsUsingDetectorTableTestPerformance(); }
  static void destroySuite( ConvertUnitsUsingDetectorTableTestPerformance *suite ) { delete suite; }

  ConvertUnitsUsingDetectorTableTestPerformance()
  {
    FrameworkManager::Instance().exec("Load","Filename=HET15869;OutputWorkspace=hist_tof");
    FrameworkManager::Instance().exec("Load","Filename=CNCS_7860_event;OutputWorkspace=event_tof");
  }

  void test_histogram_workspace()
  {
    IAlgorithm * alg;
    alg = FrameworkManager::Instance().exec("ConvertUnitsUsingDetectorTable","InputWorkspace=hist_tof;OutputWorkspace=hist_wave;Target=Wavelength");
    TS_ASSERT( alg->isExecuted() );
    alg = FrameworkManager::Instance().exec("ConvertUnitsUsingDetectorTable","InputWorkspace=hist_wave;OutputWorkspace=hist_dSpacing;Target=dSpacing");
    TS_ASSERT( alg->isExecuted() );
  }

  void test_event_workspace()
  {
    IAlgorithm * alg;
    alg = FrameworkManager::Instance().exec("ConvertUnitsUsingDetectorTable","InputWorkspace=event_tof;OutputWorkspace=event_wave;Target=Wavelength");
    TS_ASSERT( alg->isExecuted() );
    alg = FrameworkManager::Instance().exec("ConvertUnitsUsingDetectorTable","InputWorkspace=event_wave;OutputWorkspace=event_dSpacing;Target=dSpacing");
    TS_ASSERT( alg->isExecuted() );
  }

private:
  MatrixWorkspace_sptr histWS;
  MatrixWorkspace_sptr eventWS;
};

#endif /*CONVERTUNITSUSINGDETECTORTABLETEST_H_*/
