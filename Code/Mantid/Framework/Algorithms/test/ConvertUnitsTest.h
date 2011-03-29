#ifndef CONVERTUNITSTEST_H_
#define CONVERTUNITSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ConvertUnitsTest : public CxxTest::TestSuite
{
public:

  ConvertUnitsTest()
  {
  }

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
    int forSpecDetMap[256];
    for (int j = 0; j < 256; ++j) {
      space2D->setX(j, x);
      space2D->setData(j, a, e);
      // Just set the spectrum number to match the index
      space2D->getAxis(1)->spectraNo(j) = j;
      forSpecDetMap[j] = j;
    }

    // Register the workspace in the data service
    this->inputSpace = "testWorkspace";
    AnalysisDataService::Instance().add(inputSpace, space);

    // Load the instrument data
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "HET_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", this->inputSpace);
    loader.execute();

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space2D->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, 256 );

    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testExec()
  {
    this->setup_WS();
    if ( !alg.isInitialized() ) alg.initialize();

    // Set the properties
    alg.setPropertyValue("InputWorkspace",inputSpace);
    outputSpace = "outWorkspace";
    alg.setPropertyValue("OutputWorkspace",outputSpace);
    alg.setPropertyValue("Target","Wavelength");
    alg.setPropertyValue("AlignBins","1");

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

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

  void testConvertQuickly()
  {
    ConvertUnits quickly;
    quickly.initialize();
    TS_ASSERT( quickly.isInitialized() );
    quickly.setPropertyValue("InputWorkspace",outputSpace);
    quickly.setPropertyValue("OutputWorkspace","quickOut2");
    quickly.setPropertyValue("Target","Energy");
    TS_ASSERT_THROWS_NOTHING( quickly.execute() );
    TS_ASSERT( quickly.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("quickOut2")) );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "Energy");
    TS_ASSERT_DELTA( output->dataX(1)[1], 10.10, 0.01 );

    AnalysisDataService::Instance().remove("quickOut2");
  }

  void testConvertQuicklyCommonBins()
  {
    Workspace2D_sptr input = WorkspaceCreationHelper::Create2DWorkspace123(3,10,1);
    input->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    AnalysisDataService::Instance().add("quickIn", input);
    ConvertUnits quickly;
    quickly.initialize();
    TS_ASSERT( quickly.isInitialized() );
    quickly.setPropertyValue("InputWorkspace","quickIn");
    quickly.setPropertyValue("OutputWorkspace","quickOut");
    quickly.setPropertyValue("Target","dSpacing");
    TS_ASSERT_THROWS_NOTHING( quickly.execute() );
    TS_ASSERT( quickly.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("quickOut")) );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS( &(output->dataX(0)[0]), &(output->dataX(0)[0]) );
    for (MatrixWorkspace::const_iterator it(*output); it != it.end(); ++it)
    {
      TS_ASSERT_EQUALS( it->X(), 2.0*M_PI );
    }

    AnalysisDataService::Instance().remove("quickIn");
    AnalysisDataService::Instance().remove("quickOut");
  }

  void testDeltaE()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");
    Mantid::DataHandling::LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MAR11060.raw");
    loader.setPropertyValue("SpectrumList", "900");
    std::string ws = "mar";
    loader.setPropertyValue("OutputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT( loader.isExecuted() );

    ConvertUnits conv;
    conv.initialize();
    conv.setPropertyValue("InputWorkspace",ws);
    std::string outputSpace = "outWorkspace";
    conv.setPropertyValue("OutputWorkspace",outputSpace);
    conv.setPropertyValue("Target","DeltaE");
    conv.setPropertyValue("Emode","Direct");
    conv.setPropertyValue("Efixed","12");
    conv.execute();

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)) );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_EQUALS( output->blocksize(), 475 );  // Value from IDF
//    TS_ASSERT_EQUALS( output->blocksize(), 472 );  // Value from RAW geometry

    AnalysisDataService::Instance().remove(outputSpace);

    ConvertUnits conv2;
    conv2.initialize();
    conv2.setPropertyValue("InputWorkspace",ws);
    conv.setPropertyValue("OutputWorkspace",outputSpace);
    conv.setPropertyValue("Target","DeltaE_inWavenumber");
    conv.setPropertyValue("Emode","Indirect");
    conv.setPropertyValue("Efixed","10");
    conv.execute();

    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)) );
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "DeltaE_inWavenumber");
    TS_ASSERT_EQUALS( output->blocksize(), 965 );

    AnalysisDataService::Instance().remove(ws);
    AnalysisDataService::Instance().remove(outputSpace);
  }

  void setup_Event()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "SNS");
    this->inputSpace = "eventWS";
    Mantid::DataHandling::LoadEventPreNeXus loader;
    loader.initialize();
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    std::string pulsefile( "CNCS_7860_pulseid.dat" );
    pulsefile = "";
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setPropertyValue("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", this->inputSpace);
    loader.setMaxEventsToLoad(10000);
//    loader.setPropertyValue("InstrumentFilename", "CNCS_Definition.xml");
    loader.execute();
    TS_ASSERT (loader.isExecuted() );
  }

  void testExecEvent_sameOutputWS()
  {
    this->setup_Event();

    //Retrieve Workspace
    EventWorkspace_sptr WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputSpace));
    TS_ASSERT( WS ); //workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    double a_tof = WS->getEventList(0).getEvents()[0].tof();
    double a_x = WS->getEventList(0).dataX()[1];

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
    TS_ASSERT_DIFFERS(a_tof, WS->getEventList(0).getEvents()[0].tof());
    //and a X changed
    TS_ASSERT_DIFFERS(a_x, WS->getEventList(0).dataX()[1]);
  }

  void testExecEvent_TwoStepConversionWithDeltaE()
  {
    // Test to make sure the TOF->DeltaE->Other Quantity works for
    // EventWorkspaces
    this->setup_Event();

    ConvertUnits conv;
    conv.initialize();
    conv.setPropertyValue("InputWorkspace", this->inputSpace);
    conv.setPropertyValue("OutputWorkspace", this->inputSpace);
    conv.setPropertyValue("Target","DeltaE");
    conv.setPropertyValue("Emode","Direct");
    conv.setPropertyValue("Efixed","15.0");
    conv.execute();

    ConvertUnits conv2;
    conv2.initialize();
    conv2.setPropertyValue("InputWorkspace", this->inputSpace);
    conv2.setPropertyValue("OutputWorkspace", this->inputSpace);
    conv2.setPropertyValue("Target","Wavelength");
    conv2.setPropertyValue("Emode","Direct");
    conv2.setPropertyValue("Efixed","15.0");
    TS_ASSERT_THROWS_NOTHING( conv2.execute() );
    TS_ASSERT( conv2.isExecuted() );
  }

private:
  ConvertUnits alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*CONVERTUNITSTEST_H_*/
