#ifndef POWERLAWCORRECTIONTEST_H_
#define POWERLAWCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/PowerLawCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class PowerLawCorrectionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( expon.name(), "PowerLawCorrection" );
  }
  
  void testVersion()
  {
    TS_ASSERT_EQUALS( expon.version(), 1 );
  }
  
  void testCategory()
  {
    TS_ASSERT_EQUALS( expon.category(), "CorrectionFunctions" );
  }
  
  void testInit()
  {
    Mantid::Algorithms::PowerLawCorrection expon2;
    TS_ASSERT_THROWS_NOTHING( expon2.initialize() );
    TS_ASSERT( expon2.isInitialized() );

    const std::vector<Property*> props = expon2.getProperties();
    TS_ASSERT_EQUALS( props.size(), 4 );

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" );
    TS_ASSERT( props[0]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) );

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
    TS_ASSERT( props[1]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) );

    TS_ASSERT_EQUALS( props[2]->name(), "C0");
    TS_ASSERT( props[2]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[2]) );

    TS_ASSERT_EQUALS( props[3]->name(), "C1");
    TS_ASSERT( props[3]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[3]) );
  }
  
  void testMultiply()
  {
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(2,3,0.5);
    AnalysisDataService::Instance().add("PowerLawCorrectionInputWS",inputWS);
    
    Mantid::Algorithms::PowerLawCorrection expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING( expon3.setPropertyValue("InputWorkspace","PowerLawCorrectionInputWS") );
    TS_ASSERT_THROWS_NOTHING( expon3.setPropertyValue("OutputWorkspace","PowerLawCorrectionWSCor") );
    TS_ASSERT_THROWS_NOTHING( expon3.setPropertyValue("c0","3.0") );
    TS_ASSERT_THROWS_NOTHING( expon3.setPropertyValue("c1","2.0") );
    
    TS_ASSERT_THROWS_NOTHING( expon3.execute() );
    TS_ASSERT( expon3.isExecuted() );
    
    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("PowerLawCorrectionWSCor")) );
    TS_ASSERT( result );
    
    for (size_t i = 0; i < result->getNumberHistograms(); ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        double factor = 3.0*pow(j+1,2.0);
        TS_ASSERT_EQUALS( result->dataX(i)[j], inputWS->dataX(i)[j] );
        TS_ASSERT_DELTA( result->dataY(i)[j], inputWS->dataY(i)[j]*factor, 0.0001 );
        TS_ASSERT_DELTA( result->dataE(i)[j], inputWS->dataE(i)[j]*factor, 0.0001 );
      }
    }
    
    AnalysisDataService::Instance().remove("PowerLawCorrectionInputWS");
    AnalysisDataService::Instance().remove("PowerLawCorrectionWSCor");
  }
  
private:
  Mantid::Algorithms::PowerLawCorrection expon;

};

#endif /*POWERLAWCORRECTIONTEST_H_*/
