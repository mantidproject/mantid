#ifndef SUMROWCOLUMNTEST_H_
#define SUMROWCOLUMNTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SumRowColumn.h"
#include "MantidDataHandling/LoadRaw3.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class SumRowColumnTest : public CxxTest::TestSuite
{
public:

  static SumRowColumnTest *createSuite() { return new SumRowColumnTest(); }
  static void destroySuite(SumRowColumnTest *suite) { delete suite; }

  SumRowColumnTest() : summer(), inputWS("loq-front")
  {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48094.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","3");
    loader.setPropertyValue("SpectrumMax","16386");
    loader.execute();
  }
  
  ~SumRowColumnTest()
  {
    AnalysisDataService::Instance().remove(inputWS);
  }

  void testName()
  {
    TS_ASSERT_EQUALS( summer.name(), "SumRowColumn" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( summer.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( summer.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( summer.initialize() )
    TS_ASSERT( summer.isInitialized() )
  }

  void testPropertiesNotSet()
  {
    TS_ASSERT_THROWS_NOTHING( summer.setPropertyValue("InputWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( summer.setPropertyValue("OutputWorkspace","nowt") )

    TS_ASSERT_THROWS( summer.execute(), std::runtime_error )
    TS_ASSERT( ! summer.isExecuted() )
  }
	
  void testHorizontal()
  {
    Mantid::Algorithms::SumRowColumn summer2;
    TS_ASSERT_THROWS_NOTHING( summer2.initialize() )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("InputWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("OutputWorkspace","H") )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("Orientation","D_H") )
    
    TS_ASSERT_THROWS_NOTHING( summer2.execute() )
    TS_ASSERT( summer2.isExecuted() )
    
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(
                                        AnalysisDataService::Instance().retrieve("H")));
    // Check a couple of values
    TS_ASSERT_EQUALS( output->readX(0).size(), 128 )
    TS_ASSERT_EQUALS( output->readY(0).size(), 128 )
    TS_ASSERT_EQUALS( output->readX(0)[10], 10 )
    TS_ASSERT_EQUALS( output->readX(0)[99], 99 )
    TS_ASSERT_EQUALS( output->readY(0)[10], 131 )
    TS_ASSERT_EQUALS( output->readY(0)[99], 115 )
    TS_ASSERT_EQUALS( output->readE(0)[10], 0 )
    TS_ASSERT_EQUALS( output->readE(0)[99], 0 )

    TS_ASSERT( ! output->getAxis(0)->unit() )

    AnalysisDataService::Instance().remove("H");
  }
	
  void testVertical()
  {
    Mantid::Algorithms::SumRowColumn summer2;
    TS_ASSERT_THROWS_NOTHING( summer2.initialize() )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("InputWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("OutputWorkspace","V") )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("Orientation","D_V") )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("XMin","4000") )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("XMax","10000") )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("H/V_Min","50") )
    TS_ASSERT_THROWS_NOTHING( summer2.setPropertyValue("H/V_Max","100") )
    
    TS_ASSERT_THROWS_NOTHING( summer2.execute() )
    TS_ASSERT( summer2.isExecuted() )
    
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(
                                        AnalysisDataService::Instance().retrieve("V")));
    // Check a couple of values
    TS_ASSERT_EQUALS( output->readX(0).size(), 128 )
    TS_ASSERT_EQUALS( output->readY(0).size(), 128 )
    TS_ASSERT_EQUALS( output->readX(0)[10], 10 )
    TS_ASSERT_EQUALS( output->readX(0)[99], 99 )
    TS_ASSERT_EQUALS( output->readY(0)[10], 3 )
    TS_ASSERT_EQUALS( output->readY(0)[99], 5 )
    TS_ASSERT_EQUALS( output->readE(0)[10], 0 )
    TS_ASSERT_EQUALS( output->readE(0)[99], 0 )

    AnalysisDataService::Instance().remove("H");
  }
	
private:
  Mantid::Algorithms::SumRowColumn summer;
  std::string inputWS;
};

#endif /*SUMROWCOLUMNTEST_H_*/
