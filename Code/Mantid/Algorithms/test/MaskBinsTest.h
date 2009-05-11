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
  
  void testCommonBins()
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
      const MatrixWorkspace::MaskList& mask = outputWS->maskedBins(i);
      TS_ASSERT_EQUALS( mask.size(), 3 )
      MatrixWorkspace::MaskList::const_iterator it;
      int k = 20;
      for (it = mask.begin(); it != mask.end(); ++it,++k)
      {
        TS_ASSERT_EQUALS( (*it).first, k )
        TS_ASSERT_EQUALS( (*it).second, 1.0 )
      }
      
      for (int j = 0; j < outputWS->blocksize(); ++j)
      {
        if ( j >= 20 && j < 23 )
        {
          TS_ASSERT_EQUALS( outputWS->readY(i)[j], 0.0 )
          TS_ASSERT_EQUALS( outputWS->readE(i)[j], 0.0 )
        }
        else
        {
          TS_ASSERT_EQUALS( outputWS->readY(i)[j], 2.0 )
          TS_ASSERT_DELTA( outputWS->readE(i)[j], sqrt(2.0), 0.0001 )
        }
        TS_ASSERT_EQUALS( outputWS->readX(i)[j], j )     
      }
    }
    
    // Clean up
    ads.remove(workspaceName);
    ads.remove(resultWorkspaceName);
  }

  void testRaggedBins()
  {
    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(3,10,0.0);
    // Now change one set of bin boundaries so that the don't match the others
    Mantid::MantidVec& X = WS->dataX(1);
    std::transform(X.begin(),X.end(),X.begin(),std::bind2nd(std::minus<double>(),10.0));
    
    AnalysisDataService::Instance().add(workspaceName,WS);
    
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("InputWorkspace",workspaceName) )
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("OutputWorkspace",workspaceName) )
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("XMin","-11.0") )
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("XMax","-8.5") )
    
    TS_ASSERT_THROWS_NOTHING( masker2.execute() )
    TS_ASSERT( masker2.isExecuted() )
    
    TS_ASSERT( ! WS->hasMaskedBins(0) )
    TS_ASSERT( WS->hasMaskedBins(1) )
    TS_ASSERT( ! WS->hasMaskedBins(2) )
    
    const MatrixWorkspace::MaskList& mask = WS->maskedBins(1);
    TS_ASSERT_EQUALS( mask.size(), 2 )
    const Mantid::MantidVec& Y = WS->readY(1);
    const Mantid::MantidVec& E = WS->readE(1);
    MatrixWorkspace::MaskList::const_iterator it;
    int k = 0;
    for (it = mask.begin(); it != mask.end(); ++it,++k)
    {
      TS_ASSERT_EQUALS( (*it).first, k )
      TS_ASSERT_EQUALS( (*it).second, 1.0 )
      TS_ASSERT_EQUALS( Y[k], 0.0 )
      TS_ASSERT_EQUALS( E[k], 0.0 )
    }
       
    AnalysisDataService::Instance().remove(workspaceName);
  }
  
  
private:
  Mantid::Algorithms::MaskBins masker;
};

#endif /*MASKBINSTEST_H_*/
