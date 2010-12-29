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

  void testNoGravity()
  {
    Mantid::DataHandling::LoadRaw2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48098.raw");
    loader.setPropertyValue("OutputWorkspace",m_inputWS);
    loader.setPropertyValue("SpectrumMin","30");
    loader.setPropertyValue("SpectrumMax","130");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",m_inputWS);
    convert.setPropertyValue("OutputWorkspace",m_inputWS);
    convert.setPropertyValue("Target","Wavelength");
    convert.execute();
    
    if (!qxy.isInitialized()) qxy.initialize();

    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("InputWorkspace",m_inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("MaxQxy","0.1") )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("DeltaQ","0.002") )

    TS_ASSERT_THROWS_NOTHING( qxy.execute() )
    TS_ASSERT( qxy.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
             
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 100 )
    TS_ASSERT_EQUALS( result->blocksize(), 100 )
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "MomentumTransfer" )
    TS_ASSERT_EQUALS( result->getAxis(1)->unit()->unitID(), "MomentumTransfer" )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), -0.1 )
    TS_ASSERT_DELTA( (*(result->getAxis(1)))(31), -0.038, 0.001 )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(100), 0.1 )

    TS_ASSERT_EQUALS( result->readX(0).size(), 101 )
    TS_ASSERT_EQUALS( result->readX(0).front(), -0.1 )
    TS_ASSERT_DELTA( result->readX(0)[64], 0.028, 0.01 )
    TS_ASSERT_EQUALS( result->readX(0).back(), 0.1 )
    
    TS_ASSERT_DIFFERS( result->readY(0).front(), result->readY(0).front() )  // NaN
    TS_ASSERT_DELTA( result->readY(26)[73], 4438798, 1 )
    TS_ASSERT_DELTA( result->readY(18)[36], 174005, 1 )
    
    TS_ASSERT_DELTA( result->readE(20)[67], 0.0, 1e-3 )
    TS_ASSERT_DELTA( result->readE(27)[70], 233279.0150, 1e-3 )
    TS_ASSERT_DELTA( result->readE(23)[34], 0.0, 1e-3 )
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testGravity()
  {
    Mantid::Algorithms::Qxy qxy;
    qxy.initialize();

    //inputWS was set up by the previous test, not ideal but it does save a lot of CPU time!
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("InputWorkspace",m_inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("MaxQxy","0.1") )
    TS_ASSERT_THROWS_NOTHING( qxy.setPropertyValue("DeltaQ","0.002") )
    TS_ASSERT_THROWS_NOTHING( qxy.setProperty("AccountForGravity",true) )

    TS_ASSERT_THROWS_NOTHING( qxy.execute() )
    TS_ASSERT( qxy.isExecuted() )
                      
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) ) 
/*    for (int i = 0 ; i < 30; i ++)
    {
    std::cout << i << std::endl;
    for (int j = 0 ; j < 30; j ++)
    {
    std::cout << result->readY(i)[j] << " ";
    }
  }*/
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 100 )
    TS_ASSERT_EQUALS( result->blocksize(), 100 )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), -0.1 )
    TS_ASSERT_DELTA( (*(result->getAxis(1)))(31), -0.038, 0.001 )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(100), 0.1 )
  
    TS_ASSERT_DIFFERS( result->readY(0).front(), result->readY(0).front() )  // NaN
    TS_ASSERT_DELTA( result->readY(3)[26], 696467, 1 )
    TS_ASSERT_DELTA( result->readY(7)[27], 696566, 1 )
    
    TS_ASSERT_DELTA( result->readE(20)[67], 0.0, 1e-3 )
    TS_ASSERT_DELTA( result->readE(7)[27], 696565.8656, 1e-3 )
    TS_ASSERT_DELTA( result->readE(23)[34], 0.0, 1e-3 )
    
    Mantid::API::AnalysisDataService::Instance().remove(m_inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  QxyTest() : m_inputWS("QxyTest_input_in_wav") {}
private:
  Mantid::Algorithms::Qxy qxy;
  const std::string m_inputWS;

};

#endif /*QXYTEST_H_*/
