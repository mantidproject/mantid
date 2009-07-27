#ifndef QXYTEST_H_
#define QXYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Qxy.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidAlgorithms/ConvertUnits.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class QxyTest : public CxxTest::TestSuite
{
public:
  QxyTest() : qxy(), inputWS("wav")
  {
    Mantid::DataHandling::LoadRaw2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/LOQ48098.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","30");
    loader.setPropertyValue("SpectrumMax","130");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",inputWS);
    convert.setPropertyValue("OutputWorkspace",inputWS);
    convert.setPropertyValue("Target","Wavelength");
    //convert.setPropertyValue("AlignBins","1");
    convert.execute();
  }
  
  void testName()
  {
    TS_ASSERT_EQUALS( qxy.name(), "Qxy" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( qxy.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( qxy.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( qxy.initialize() )
    TS_ASSERT( qxy.isInitialized() )
  }
	
  void testExec()
  {
    if (!qxy.isInitialized()) qxy.initialize();
	  
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("InputWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("OutputWorkspace","result") )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("MaxQxy","0.1") )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("DeltaQ","0.002") )
	  
    TS_ASSERT_THROWS_NOTHING( qxy.execute() )
    TS_ASSERT( qxy.isExecuted() )
  }
	
private:
  Mantid::Algorithms::Qxy qxy;
  std::string inputWS;
};

#endif /*QXYTEST_H_*/
