#ifndef MASKBINSTEST_H_
#define MASKBINSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/MaskBins.h"

using namespace Mantid::API;

class MaskBinsTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( masker.name(), "MaskBins")
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( masker.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( masker.category(), "General" )
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( masker.initialize() )
    TS_ASSERT( masker.isInitialized() )
  }
  
  void testExec()
  {
    if (!masker.isInitialized()) masker.initialize();
    
    // Create a dummy workspace
    const std::string workspaceName("forMasking");
    const std::string resultWorkspaceName("masked");
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();
    ads.add(workspaceName,WorkspaceCreationHelper::Create2DWorkspaceBinned(5,25,0.0));
    
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("InputWorkspace",workspaceName) )
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("OutputWorkspace",resultWorkspaceName) )
    
    // Check that execution fails if XMin & XMax not set
    TS_ASSERT_THROWS( masker.execute(), std::runtime_error )
    TS_ASSERT( ! masker.isExecuted() )
    
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("XMin","20.0") )
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("XMax","22.5") )
    
    TS_ASSERT_THROWS_NOTHING( masker.execute() )
    TS_ASSERT( masker.isExecuted() )
    
    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ads.retrieve(resultWorkspaceName));
    
    for (int i = 0; i < outputWS->getNumberHistograms(); ++i)
    {
      TS_ASSERT( outputWS->hasMaskedBins(i) )
    }
    
    // Clean up
    ads.remove(workspaceName);
    ads.remove(resultWorkspaceName);
  }

private:
  Mantid::Algorithms::MaskBins masker;
};

#endif /*MASKBINSTEST_H_*/
