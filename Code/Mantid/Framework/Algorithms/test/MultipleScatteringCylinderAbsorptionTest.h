#ifndef MULTIPLE_SCATTERING_ABSORPTION_TEST_H_
#define MULTIPLE_SCATTERING_ABSORPTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAlgorithms/MultipleScatteringCylinderAbsorption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::MantidVec;

class MultipleScatteringCylinderAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( algorithm.name(), "MultipleScatteringCylinderAbsorption" );
  }
  
  void testVersion()
  {
    TS_ASSERT_EQUALS( algorithm.version(), 1 );
  }
  
  void testInit()
  {
    Mantid::Algorithms::MultipleScatteringCylinderAbsorption   algorithm_b;
    TS_ASSERT_THROWS_NOTHING( algorithm_b.initialize() );
    TS_ASSERT( algorithm_b.isInitialized() );

    const std::vector<Property*> props = algorithm_b.getProperties();
    TS_ASSERT_EQUALS( props.size(), 6 );

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" );
    TS_ASSERT( props[0]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) );

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
    TS_ASSERT( props[1]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) );

    TS_ASSERT_EQUALS( props[2]->name(), "AttenuationXSection");
    TS_ASSERT( props[2]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[2]) );

    TS_ASSERT_EQUALS( props[3]->name(), "ScatteringXSection");
    TS_ASSERT( props[3]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) );

    TS_ASSERT_EQUALS( props[4]->name(), "SampleNumberDensity");
    TS_ASSERT( props[4]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[4]) );


    TS_ASSERT_EQUALS( props[5]->name(), "CylinderSampleRadius");
    TS_ASSERT( props[5]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[5]) );

  }

  void testCalculationHist()
  {
    // setup the test workspace
    Workspace2D_sptr wksp = WorkspaceCreationHelper::Create2DWorkspaceBinned(9,16,1000,1000);
    wksp->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(1));
    AnalysisDataService::Instance().add("TestInputWS",wksp);

    // create and execute the algorithm
    Mantid::Algorithms::MultipleScatteringCylinderAbsorption   algorithm_c;
    TS_ASSERT_THROWS_NOTHING(algorithm_c.initialize() );
    TS_ASSERT( algorithm_c.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "InputWorkspace","TestInputWS" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "OutputWorkspace","TestOutputWS" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "CylinderSampleRadius", "0.3175" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "AttenuationXSection", "2.8" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "SampleNumberDensity", "0.0721" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "ScatteringXSection", "5.1" ) );

    TS_ASSERT_THROWS_NOTHING( algorithm_c.execute() );
    TS_ASSERT( algorithm_c.isExecuted() );

    MatrixWorkspace_sptr test_output_WS;

    TS_ASSERT_THROWS_NOTHING( test_output_WS=AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("TestOutputWS") );
    TS_ASSERT( test_output_WS );

    // setup expected values
    std::vector<double> y_expected;
    y_expected.push_back(2.22389);
    y_expected.push_back(2.2924);
    y_expected.push_back(2.36292);
    y_expected.push_back(2.43552);
    y_expected.push_back(2.51024);
    y_expected.push_back(2.58716);
    y_expected.push_back(2.66632);
    y_expected.push_back(2.7478);
    y_expected.push_back(2.83166);
    y_expected.push_back(2.91796);
    y_expected.push_back(3.00678);
    y_expected.push_back(3.0982);
    y_expected.push_back(3.19228);
    y_expected.push_back(3.28912);
    y_expected.push_back(3.38879);
    y_expected.push_back(3.49139);

    // do the final comparison
    const MantidVec & y_actual = test_output_WS->readY(0);
    for ( size_t i = 0; i < y_expected.size(); i++ )
      TS_ASSERT_DELTA( y_actual[i], y_expected[i], 0.00001 );

    // cleanup
    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("TestOutputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");
  }

  void testCalculationEvent()
  {
    // setup the test workspace
    EventWorkspace_sptr wksp = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1,1,false);

    // create the algorithm
    Mantid::Algorithms::MultipleScatteringCylinderAbsorption   algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize() );
    TS_ASSERT( algorithm.isInitialized() );

    // execute the algorithm
    std::string outName("MultipleScatteringCylinderAbsorptionEventOutput");
    TS_ASSERT_THROWS_NOTHING( algorithm.setProperty("InputWorkspace",wksp));
    TS_ASSERT_THROWS_NOTHING( algorithm.setPropertyValue("OutputWorkspace",outName));
    TS_ASSERT_THROWS_NOTHING( algorithm.execute() );
    TS_ASSERT( algorithm.isExecuted() );

    // quick checks on the output workspace
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS=AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outName));
    TS_ASSERT( boost::dynamic_pointer_cast<EventWorkspace>(outputWS));

    // do the final comparison - this is done by bounding
    const MantidVec & y_actual = outputWS->readY(0);
    TS_ASSERT_EQUALS(y_actual.size(), 100);
    for (size_t i = 0; i < y_actual.size(); ++i)
    {
      TS_ASSERT_LESS_THAN( 2.1248,      y_actual[i]);
      TS_ASSERT_LESS_THAN(y_actual[i], 2.1313);
    }

    // cleanup
    AnalysisDataService::Instance().remove(outName);
  }

private:
  Mantid::Algorithms::MultipleScatteringCylinderAbsorption algorithm;

};

#endif /*MULTIPLE_SCATTERING_ABSORPTION_TEST_H_*/
