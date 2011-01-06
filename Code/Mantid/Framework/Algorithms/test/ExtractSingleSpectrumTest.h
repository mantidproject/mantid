#ifndef EXTRACTSINGLESPECTRUMTEST_H_
#define EXTRACTSINGLESPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class ExtractSingleSpectrumTest : public CxxTest::TestSuite
{
public:
	void testName()
  {
	  TS_ASSERT_EQUALS( extractor.name(), "ExtractSingleSpectrum" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( extractor.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( extractor.category(), "General" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( extractor.initialize() )
    TS_ASSERT( extractor.isInitialized() )
    
    TS_ASSERT_EQUALS( extractor.getProperties().size(), 3 )
  }
  
  void testExec()
  {
    using namespace Mantid::API;
    
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspace154(5,5);
    const int spectrum = 2;
    for (int i=0; i<5; ++i)
    {
      inputWS->dataX(spectrum)[i] = i;
      inputWS->dataY(spectrum)[i] = 20-i;
      inputWS->dataE(spectrum)[i] = 7;
    }
    inputWS->getAxis(1)->spectraNo(spectrum) = spectrum;
    AnalysisDataService::Instance().add("input",inputWS);
    
    TS_ASSERT_THROWS_NOTHING( extractor.setPropertyValue("InputWorkspace","input") )
    TS_ASSERT_THROWS_NOTHING( extractor.setPropertyValue("OutputWorkspace","output") )
    TS_ASSERT_THROWS_NOTHING( extractor.setProperty("WorkspaceIndex",spectrum) )
    
    TS_ASSERT_THROWS_NOTHING( extractor.execute() )
    TS_ASSERT( extractor.isExecuted() )
    
    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("output"); )
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT( outputWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(output) )
    TS_ASSERT_EQUALS( outputWS->blocksize(), 5 )
    TS_ASSERT_EQUALS( outputWS->readX(0).size(), 5 )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), spectrum )
    for (int j=0; j<5; ++j)
    {
      TS_ASSERT_EQUALS( outputWS->readX(0)[j], j )
      TS_ASSERT_EQUALS( outputWS->readY(0)[j], 20-j )
      TS_ASSERT_EQUALS( outputWS->readE(0)[j], 7 )
    }
    
    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("output");
  }
  
private:
  Mantid::Algorithms::ExtractSingleSpectrum extractor; 
};

#endif /*EXTRACTSINGLESPECTRUMTEST_H_*/
