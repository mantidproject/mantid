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
    TS_ASSERT_EQUALS( algorithm.name(), "MultipleScatteringAbsorption" )
  }
  
  void testVersion()
  {
    TS_ASSERT_EQUALS( algorithm.version(), 1 )
  }
  
  void testCategory()
  {
    TS_ASSERT_EQUALS( algorithm.category(), "General" )
  }
  
  void testInit()
  {
    Mantid::Algorithms::MultipleScatteringAbsorption   algorithm_b;
    TS_ASSERT_THROWS_NOTHING( algorithm_b.initialize() )
    TS_ASSERT( algorithm_b.isInitialized() )

    const std::vector<Property*> props = algorithm_b.getProperties();
    TS_ASSERT_EQUALS( props.size(), 9 )

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" )
    TS_ASSERT( props[0]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) )

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" )
    TS_ASSERT( props[1]->isDefault() )
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) )

    TS_ASSERT_EQUALS( props[2]->name(), "WorkspaceIndex")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<int>* >(props[2]) )

    TS_ASSERT_EQUALS( props[3]->name(), "total_path")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )

    TS_ASSERT_EQUALS( props[4]->name(), "angle_deg")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )

    TS_ASSERT_EQUALS( props[5]->name(), "radius")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )

    TS_ASSERT_EQUALS( props[6]->name(), "coeff1")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )

    TS_ASSERT_EQUALS( props[7]->name(), "coeff2")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )

    TS_ASSERT_EQUALS( props[8]->name(), "coeff3")
    TS_ASSERT( props[2]->isDefault() )
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) )
  }

  void testZeroing()
  {                                               // load input and "Gold" result workspaces
    Mantid::NeXus::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","../../../../Test/Data/PG3_539_Pixel_1.nxs");
    loader.setProperty("OutputWorkspace","TestInputWS");
    loader.execute();

    loader.initialize();
    loader.setProperty("Filename","../../../../Test/Data/PG3_539_Pixel_1_Result_MultScatAbs.nxs");
    loader.setProperty("OutputWorkspace","MultScatAbsGoldWS");
    loader.execute();
                                                 // create and execute the algorithm
    Mantid::Algorithms::MultipleScatteringAbsorption   algorithm_c;
    TS_ASSERT_THROWS_NOTHING(algorithm_c.initialize() );
    TS_ASSERT( algorithm_c.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "InputWorkspace","TestInputWS" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "OutputWorkspace","TestOutputWS" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "WorkspaceIndex", "0" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "total_path", "62.602" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "angle_deg", "129.824" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "radius", "0.3175" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "coeff1", "2.8" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "coeff2", "0.0721" ) );
    TS_ASSERT_THROWS_NOTHING( algorithm_c.setPropertyValue( "coeff3", "5.1" ) );

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
