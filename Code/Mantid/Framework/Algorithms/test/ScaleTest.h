#ifndef SCALETEST_H_
#define SCALETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Scale.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVec;

class ScaleTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( scale.name(), "Scale" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( scale.version(), 1 )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( scale.initialize() )
    TS_ASSERT( scale.isInitialized() )
  }

  void testMultiply()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    if (!scale.isInitialized()) scale.initialize();

    AnalysisDataService::Instance().add("tomultiply",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("InputWorkspace","tomultiply") );
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("OutputWorkspace","multiplied") );
    TS_ASSERT_THROWS_NOTHING( scale.setPropertyValue("Factor","2.5") );

    TS_ASSERT_THROWS_NOTHING( scale.execute() );
    TS_ASSERT( scale.isExecuted() );

    MatrixWorkspace_const_sptr in,result;
    TS_ASSERT_THROWS_NOTHING( in = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("tomultiply")) );
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("multiplied")) );

    testScaleFactorApplied(in, result, 2.5, true); //multiply=true

    AnalysisDataService::Instance().remove("tomultiply");
    AnalysisDataService::Instance().remove("multiplied");
  }

  void testAdd()
  {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::Scale scale2;
    scale2.initialize();

    AnalysisDataService::Instance().add("toadd",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("InputWorkspace","toadd") );
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("OutputWorkspace","added") );
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("Factor","-100.0") );
    TS_ASSERT_THROWS_NOTHING( scale2.setPropertyValue("Operation","Add") );

    TS_ASSERT_THROWS_NOTHING( scale2.execute() );
    TS_ASSERT( scale2.isExecuted() );

    MatrixWorkspace_const_sptr in,result;
    TS_ASSERT_THROWS_NOTHING( in = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("toadd")) );
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("added")) );

    testScaleFactorApplied(in, result, -100, false); //multiply=false

    AnalysisDataService::Instance().remove("toadd");
    AnalysisDataService::Instance().remove("added");
  }

private:
  void testScaleFactorApplied(const Mantid::API::MatrixWorkspace_const_sptr & inputWS,
                              const Mantid::API::MatrixWorkspace_const_sptr & outputWS,
                              double factor, bool multiply)
  {
    const size_t xsize = outputWS->blocksize();
    for(size_t i = 0; i < outputWS->getNumberHistograms(); ++i)
    {
      for(size_t j = 0; j < xsize; ++j)
      {
        TS_ASSERT_DELTA(outputWS->readX(i)[j], inputWS->readX(i)[j], 1e-12);
        double resultY = (multiply) ? factor*inputWS->readY(i)[j] : factor + inputWS->readY(i)[j];
        TS_ASSERT_DELTA(outputWS->readY(i)[j], resultY, 1e-12);
        double resultE = (multiply) ? factor*inputWS->readE(i)[j] : inputWS->readE(i)[j];
        TS_ASSERT_DELTA(outputWS->readE(i)[j], resultE, 1e-12);
      }
    }
  }


  Mantid::Algorithms::Scale scale;
};

#endif /*SCALETEST_H_*/
