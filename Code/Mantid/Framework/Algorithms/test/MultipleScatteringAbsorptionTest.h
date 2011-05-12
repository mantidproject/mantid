#ifndef MULTIPLE_SCATTERING_ABSORPTION_TEST_H_
#define MULTIPLE_SCATTERING_ABSORPTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAlgorithms/MultipleScatteringAbsorption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::MantidVec;

class MultipleScatteringAbsorptionTest : public CxxTest::TestSuite
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
  
  void testCategory()
  {
    TS_ASSERT_EQUALS( algorithm.category(), "Absorption Corrections" );
  }
  
  void testInit()
  {
    Mantid::Algorithms::MultipleScatteringAbsorption   algorithm_b;
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

  void testCalculation()
  {
    // setup the test workspace
    Workspace2D_sptr wksp = WorkspaceCreationHelper::Create2DWorkspaceBinned(9,16,1000,1000);
    std::vector<int64_t> specdetmap;
    for(size_t i = 0; i < 9; ++i)
      specdetmap.push_back(i+1);
    wksp->mutableSpectraMap().clear();
    wksp->mutableSpectraMap().populateWithVector(specdetmap);
    wksp->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(1));
    AnalysisDataService::Instance().add("TestInputWS",wksp);

    // create and execute the algorithm
    Mantid::Algorithms::MultipleScatteringAbsorption   algorithm_c;
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
    MatrixWorkspace_sptr gold_output_WS;

    TS_ASSERT_THROWS_NOTHING( test_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("TestOutputWS")) );
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
    const MantidVec & y_actual = test_output_WS->dataY(0);
    for ( size_t i = 0; i < y_expected.size(); i++ )
      TS_ASSERT_DELTA( y_actual[i], y_expected[i], 0.00001 );

    // cleanup
    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("TestOutputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");
  }

private:
  Mantid::Algorithms::MultipleScatteringAbsorption algorithm;

};

#endif /*MULTIPLE_SCATTERING_ABSORPTION_TEST_H_*/
