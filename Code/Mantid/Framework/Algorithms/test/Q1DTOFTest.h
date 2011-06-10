#ifndef Q1DTEST_H_
#define Q1DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Q1DTOF.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadRKH.h"
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class Q1DTOFTest : public CxxTest::TestSuite
{
public:
  void testStatics()
  {
    Mantid::Algorithms::Q1DTOF Q1DTOF;
    TS_ASSERT_EQUALS( Q1DTOF.name(), "Q1DTOF" )
    TS_ASSERT_EQUALS( Q1DTOF.version(), 1 )
    TS_ASSERT_EQUALS( Q1DTOF.category(), "SANS" )
  }

  ///Test that we can run without the optional workspacespace
  void testNoPixelAdj()
  {
    Mantid::Algorithms::Q1DTOF Q1DTOF;
    Q1DTOF.initialize();

    const std::string outputWS("Q1DTOFTest_result");
    TS_ASSERT_THROWS_NOTHING(
      Q1DTOF.setProperty("DetBankWorkspace", m_inputWS);
      Q1DTOF.setProperty("WavelengthAdj", m_wavNorm);
      Q1DTOF.setPropertyValue("OutputWorkspace",outputWS);
      Q1DTOF.setPropertyValue("OutputBinning","0,0.02,0.5");
      // property PixelAdj is undefined but that shouldn't cause this to throw
      Q1DTOF.execute()
    )

    TS_ASSERT( Q1DTOF.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_EQUALS( result->isDistribution(), true )
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "MomentumTransfer" )
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 1 )

    TS_ASSERT_EQUALS( result->readX(0).size(), 26 )
    TS_ASSERT_DELTA( result->readX(0).front(), 0, 1e-5 )
    TS_ASSERT_DELTA( result->readX(0)[6], 0.12, 1e-5)
    TS_ASSERT_DELTA( result->readX(0).back(), 0.5, 1e-5 )

    //values below taken from running the algorithm in the state it was excepted by the ISIS SANS in
    //empty bins are 0/0
    TS_ASSERT( boost::math::isnan(result->readY(0).front()) )
    TS_ASSERT_DELTA( result->readY(0)[8], 0.30320397, 1e-7 )
    TS_ASSERT_DELTA( result->readY(0)[12], 3.65424898, 1e-7 )
    TS_ASSERT( boost::math::isnan(result->readY(0).back()) )

    TS_ASSERT( boost::math::isnan(result->readE(0).front()) )
    TS_ASSERT_DELTA( result->readE(0)[10], 8.626009e-005, 1e-9 )
    TS_ASSERT_DELTA( result->readE(0)[12], 0.0039833458, 1e-7 )
    TS_ASSERT( boost::math::isnan(result->readE(0).back()) )
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }
    
  void testPixelAdj()
  {
    Mantid::Algorithms::Q1DTOF Q1DTOF;
    Q1DTOF.initialize();

    TS_ASSERT_THROWS_NOTHING(
      Q1DTOF.setProperty("DetBankWorkspace", m_inputWS);
      Q1DTOF.setProperty("WavelengthAdj", m_wavNorm);
      Q1DTOF.setPropertyValue("PixelAdj", m_pixel);
      Q1DTOF.setPropertyValue("OutputWorkspace", m_noGrav);
      Q1DTOF.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
    )
    //#default is don't correct for gravity

    TS_ASSERT_THROWS_NOTHING( Q1DTOF.execute() )
    TS_ASSERT( Q1DTOF.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(m_noGrav)) )
    TS_ASSERT(result)
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 1 )
    
    TS_ASSERT_EQUALS( result->readX(0).size(), 83 )
    TS_ASSERT_EQUALS( result->readX(0).front(), 0.1 )
    TS_ASSERT_DELTA( result->readX(0)[3], 0.1061208, 1e-6 )
    TS_ASSERT_DELTA( result->readX(0)[56], 0.3031165, 1e-5 )
    TS_ASSERT_EQUALS( result->readX(0).back(), 0.5 )

    TS_ASSERT_DELTA( result->readY(0).front(), 0.0, 1e-5 )
    TS_ASSERT_DELTA( result->readY(0)[3], 0.38593102, 1e-5 )
    TS_ASSERT_DELTA( result->readY(0)[13], 0.29659477, 1e-5 )
    TS_ASSERT_DELTA( result->readY(0)[16], 1.48662636, 1e-5 )
    TS_ASSERT_DELTA( result->readY(0)[20], 0.0, 1 )

    //empty bins are 0/0
    std::cout << result->readE(0).front();
    TS_ASSERT_DELTA( result->readE(0).front(), 0.0, 1e-5 )
    TS_ASSERT_DELTA( result->readE(0)[10], 0.00046423408, 1e-8 )
    TS_ASSERT( boost::math::isnan(result->readE(0).back()) )
    
    Mantid::API::AnalysisDataService::Instance().remove(m_noGrav);
  }
    
  void testGravity()
  {
    Mantid::Algorithms::Q1DTOF Q1DTOF;
    TS_ASSERT_THROWS_NOTHING( Q1DTOF.initialize() );
    TS_ASSERT( Q1DTOF.isInitialized() )

    const std::string outputWS("Q1DTOFTest_result");
    TS_ASSERT_THROWS_NOTHING(
      Q1DTOF.setProperty("DetBankWorkspace", m_inputWS);
      Q1DTOF.setProperty("WavelengthAdj", m_wavNorm);
      Q1DTOF.setPropertyValue("PixelAdj", m_pixel);
      Q1DTOF.setPropertyValue("OutputWorkspace", outputWS);
      Q1DTOF.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
      Q1DTOF.setPropertyValue("AccountForGravity", "1");
      
      Q1DTOF.execute()
    )
    TS_ASSERT( Q1DTOF.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr gravity, refNoGrav = 
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT_THROWS_NOTHING(
      gravity = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
        (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )

    TS_ASSERT(refNoGrav)
    TS_ASSERT_EQUALS( (*(gravity->getAxis(1)))(0), (*(refNoGrav->getAxis(1)))(0) )
    
    TS_ASSERT_EQUALS( gravity->readX(0).size(), refNoGrav->readX(0).size() )
    TS_ASSERT_EQUALS( gravity->readX(0)[55], refNoGrav->readX(0)[55] )

    TS_ASSERT_DELTA( gravity->readY(0)[3], 0.38593103, 1e-6 )
    TS_ASSERT_DELTA( gravity->readY(0)[13], 0.29659477, 1e-6 )
    TS_ASSERT_DELTA( gravity->readY(0)[16], 1.4866264, 1e-6 )
    TS_ASSERT_DELTA( gravity->readY(0)[43], 0.076000849, 1e-8 )
    TS_ASSERT( boost::math::isnan(gravity->readY(0).back()) )

    TS_ASSERT_DELTA( gravity->readE(0).front(), 0.0, 1e-8 )
    TS_ASSERT_DELTA( gravity->readE(0)[10], 0.000464234078, 1e-8  )
    TS_ASSERT( boost::math::isnan(gravity->readE(0)[77]) )
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }
    
  void testInvalidInput()
  {
    Mantid::Algorithms::Q1DTOF Q1DTOF;
    Q1DTOF.initialize();
    
    //this is a small change to the normalization workspace that should be enough to stop progress
    Mantid::MantidVec & xData = m_wavNorm->dataX(0);
    xData[15] += 0.001;

    const std::string outputWS("Q1DTOFTest_invalid_result");
    TS_ASSERT_THROWS_NOTHING(
      Q1DTOF.setProperty("DetBankWorkspace", m_inputWS);
      Q1DTOF.setProperty("WavelengthAdj", m_wavNorm);
      Q1DTOF.setPropertyValue("OutputWorkspace", outputWS);
      Q1DTOF.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
      Q1DTOF.setPropertyValue("AccountForGravity", "1");
    )

    TS_ASSERT( ! Q1DTOF.isExecuted() )
  }
    

void createInputWorkspaces()
  {
    std::string wsName("Q1DTOFTest_inputworkspace"), wavNorm("Q1DTOFTest_wave");

    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ54431.raw");

    loader.setPropertyValue("OutputWorkspace", wavNorm);
    loader.setProperty("LoadLogFiles", false);
    loader.setPropertyValue("SpectrumList","1,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",wavNorm);
    convert.setPropertyValue("OutputWorkspace",wavNorm);
    convert.setPropertyValue("Target","Wavelength");
    convert.execute();

    Mantid::Algorithms::Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", wavNorm);
    rebin.setPropertyValue("OutputWorkspace", wavNorm);
    rebin.setPropertyValue("Params","0,0.5,30");
    rebin.execute();
    
    Mantid::Algorithms::CropWorkspace crop;
    crop.initialize();
    crop.setPropertyValue("InputWorkspace", wavNorm);
    crop.setPropertyValue("OutputWorkspace",wsName);
    crop.setPropertyValue("StartWorkspaceIndex","1");
    crop.execute();
    
    crop.setPropertyValue("InputWorkspace", wavNorm);
    crop.setPropertyValue("OutputWorkspace", wavNorm);
    crop.setPropertyValue("StartWorkspaceIndex","0");
    crop.setPropertyValue("EndWorkspaceIndex","0");
    crop.execute();

    m_inputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
    m_wavNorm = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wavNorm));

    LoadRKH loadRkh;
    loadRkh.initialize();
    loadRkh.setPropertyValue("Filename", "FLAT_CELL.061");
    loadRkh.setPropertyValue("OutputWorkspace", m_pixel);
    loadRkh.setPropertyValue("FirstColumnValue","SpectrumNumber");
    loadRkh.execute();
    crop.setPropertyValue("InputWorkspace",m_pixel);
    crop.setPropertyValue("OutputWorkspace",m_pixel);
    crop.setPropertyValue("StartWorkspaceIndex","5");
    crop.setPropertyValue("EndWorkspaceIndex","20");
    crop.execute();
    
  }
  
  ///stop the constructor from being run every time algorithms test suit is initialised
  static Q1DTOFTest *createSuite() { return new Q1DTOFTest(); }
  static void destroySuite(Q1DTOFTest *suite) { delete suite; }
  Q1DTOFTest() : m_noGrav("Q1DTOFTest_no_gravity_result"), m_pixel("Q1DTOFTest_flat_file")
  {
    createInputWorkspaces();
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_inputWS, m_wavNorm;
  std::string m_noGrav, m_pixel;
};

#endif /*Q1DTEST_H_*/
