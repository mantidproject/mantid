#ifndef Q1D2Test_H_
#define Q1D2Test_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Q1D2.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadRKH.h"
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class Q1D2Test : public CxxTest::TestSuite
{
public:
  void testStatics()
  {
    Mantid::Algorithms::Q1D2 Q1D2;
    TS_ASSERT_EQUALS( Q1D2.name(), "Q1D" )
    TS_ASSERT_EQUALS( Q1D2.version(), 2 )
    TS_ASSERT_EQUALS( Q1D2.category(), "SANS" )
  }

  ///Test that we can run without the optional workspacespace
  void xtestNoPixelAdj()
  {
    Mantid::Algorithms::Q1D2 Q1D2;
    Q1D2.initialize();

    const std::string outputWS("Q1D2Test_result");
    TS_ASSERT_THROWS_NOTHING(
      Q1D2.setProperty("DetBankWorkspace", m_inputWS);
      Q1D2.setProperty("WavelengthAdj", m_wavNorm);
      Q1D2.setPropertyValue("OutputWorkspace",outputWS);
      Q1D2.setPropertyValue("OutputBinning","0,0.02,0.5");
      // property PixelAdj is undefined but that shouldn't cause this to throw
      Q1D2.execute()
    )
        /*std::string s;
    std::getline(std::cin, s);*/

    TS_ASSERT( Q1D2.isExecuted() )
    
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
    
  void xtestPixelAdj()
  {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();

    TS_ASSERT_THROWS_NOTHING(
      Q1D.setProperty("DetBankWorkspace", m_inputWS);
      Q1D.setProperty("WavelengthAdj", m_wavNorm);
      Q1D.setPropertyValue("PixelAdj", m_pixel);
      Q1D.setPropertyValue("OutputWorkspace", m_noGrav);
      Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
    )
    //#default is don't correct for gravity
    TS_ASSERT_THROWS_NOTHING( Q1D.execute() )
    TS_ASSERT( Q1D.isExecuted() )
    
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
    
  void xtestGravity()
  {
    Mantid::Algorithms::Q1D2 Q1D;
    TS_ASSERT_THROWS_NOTHING( Q1D.initialize() );
    TS_ASSERT( Q1D.isInitialized() )

    const std::string outputWS("Q1D2Test_result");
    TS_ASSERT_THROWS_NOTHING(
      Q1D.setProperty("DetBankWorkspace", m_inputWS);
      Q1D.setProperty("WavelengthAdj", m_wavNorm);
      Q1D.setPropertyValue("PixelAdj", m_pixel);
      Q1D.setPropertyValue("OutputWorkspace", outputWS);
      Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
      Q1D.setPropertyValue("AccountForGravity", "1");
      
      Q1D.execute()
    )
    TS_ASSERT( Q1D.isExecuted() )
    
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
    
  void xtestInvalidInput()
  {
    Mantid::Algorithms::Q1D2 Q1D;
    Q1D.initialize();
    
    //this is a small change to the normalization workspace that should be enough to stop progress
    Mantid::MantidVec & xData = m_wavNorm->dataX(0);
    xData[15] += 0.001;

    const std::string outputWS("Q1D2Test_invalid_result");
    TS_ASSERT_THROWS_NOTHING(
      Q1D.setProperty("DetBankWorkspace", m_inputWS);
      Q1D.setProperty("WavelengthAdj", m_wavNorm);
      Q1D.setPropertyValue("OutputWorkspace", outputWS);
      Q1D.setPropertyValue("OutputBinning", "0.1,-0.02,0.5");
      Q1D.setPropertyValue("AccountForGravity", "1");
    )

    TS_ASSERT( ! Q1D.isExecuted() )
  }
    

void createInputWorkspaces()
  {
    std::string wsName("Q1D2Test_inputworkspace"), wavNorm("Q1D2Test_wave");

    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48097.raw");

    loader.setPropertyValue("OutputWorkspace", wavNorm);
    loader.setProperty("LoadLogFiles", false);
    loader.setPropertyValue("SpectrumMin","8603");
    loader.setPropertyValue("SpectrumMax","8632");
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
    crop.setPropertyValue("StartWorkspaceIndex","8603");
    crop.setPropertyValue("EndWorkspaceIndex","8632");
    crop.execute();
    
  }
  
  ///stop the constructor from being run every time algorithms test suit is initialised
  static Q1D2Test *createSuite() { return new Q1D2Test(); }
  static void destroySuite(Q1D2Test *suite) { delete suite; }
  Q1D2Test() : m_noGrav("Q1D2Test_no_gravity_result"), m_pixel("Q1DTest_flat_file")
  {
//need another file in the repository for this
//    createInputWorkspaces();
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_inputWS, m_wavNorm;
  std::string m_noGrav, m_pixel;
};

#endif /*Q1D2Test_H_*/
