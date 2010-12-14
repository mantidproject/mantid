#ifndef MULTIPLE_SCATTERING_ABSORPTION_TEST_H_
#define MULTIPLE_SCATTERING_ABSORPTION_TEST_H_

#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/MultipleScatteringAbsorption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidNexus/LoadNexusProcessed.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class MultipleScatteringAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( algorithm.name(), "MultipleScatteringCylinderAbsorption" )
  }
  
  void testVersion()
  {
    TS_ASSERT_EQUALS( algorithm.version(), 1 )
  }
  
  void testCategory()
  {
    TS_ASSERT_EQUALS( algorithm.category(), "Absorption Corrections" )
  }
  
  void testInit()
  {
    Mantid::Algorithms::MultipleScatteringAbsorption   algorithm_b;
    TS_ASSERT_THROWS_NOTHING( algorithm_b.initialize() )
    TS_ASSERT( algorithm_b.isInitialized() )

    const std::vector<Property*> props = algorithm_b.getProperties();
    TS_ASSERT_EQUALS( props.size(), 6 )

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) )

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" )
    TS_ASSERT( props[1]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) )

    TS_ASSERT_EQUALS( props[2]->name(), "AttenuationXSection")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[2]) )

    TS_ASSERT_EQUALS( props[3]->name(), "ScatteringXSection")
    TS_ASSERT( props[3]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )

    TS_ASSERT_EQUALS( props[4]->name(), "SampleNumberDensity")
    TS_ASSERT( props[4]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[4]) )


    TS_ASSERT_EQUALS( props[5]->name(), "CylinderSampleRadius")
    TS_ASSERT( props[5]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[5]) )

  }

  void xtestCalculation()
  {                                               // load input and "Gold" result workspaces
    Mantid::NeXus::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","../../../../Test/AutoTestData/PG3_539_Pixel_1.nxs");
    loader.setProperty("OutputWorkspace","TestInputWS");
    loader.execute();

    loader.initialize();
    loader.setProperty("Filename","../../../../Test/AutoTestData/PG3_539_Pixel_1_Result_MultScatAbs.nxs");
    loader.setProperty("OutputWorkspace","MultScatAbsGoldWS");
    loader.execute();
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

    TS_ASSERT_THROWS_NOTHING( test_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("TestOutputWS")) )
    TS_ASSERT( test_output_WS )

    TS_ASSERT_THROWS_NOTHING( gold_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("MultScatAbsGoldWS")) )
    TS_ASSERT( gold_output_WS )

    TS_ASSERT_EQUALS( test_output_WS->size(), gold_output_WS->size() );

    for ( int i = 0; i < test_output_WS->size(); i++ )
      TS_ASSERT_DELTA( test_output_WS->dataY(0)[i], gold_output_WS->dataY(0)[i], 0.00001 )

    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");
  }

private:
  Mantid::Algorithms::MultipleScatteringAbsorption algorithm;

};

#endif /*MULTIPLE_SCATTERING_ABSORPTION_TEST_H_*/
