#ifndef MASKBINSTEST_H_
#define MASKBINSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/MaskBins.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class MaskBinsTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( masker.name(), "MaskBins");
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( masker.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( masker.category(), "General" );
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( masker.initialize() );
    TS_ASSERT( masker.isInitialized() );
  }
  
  void testCommonBins()
  {
    if (!masker.isInitialized()) masker.initialize();
    
    // Create a dummy workspace
    const std::string workspaceName("forMasking");
    const std::string resultWorkspaceName("masked");
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();
    ads.add(workspaceName,WorkspaceCreationHelper::Create2DWorkspaceBinned(5,25,0.0));
    
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("InputWorkspace",workspaceName) );
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("OutputWorkspace",resultWorkspaceName) );
    
    // Check that execution fails if XMin & XMax not set
    TS_ASSERT_THROWS( masker.execute(), std::runtime_error );
    TS_ASSERT( ! masker.isExecuted() );
    
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("XMin","20.0") );
    TS_ASSERT_THROWS_NOTHING( masker.setPropertyValue("XMax","22.5") );
    
    TS_ASSERT_THROWS_NOTHING( masker.execute() );
    TS_ASSERT( masker.isExecuted() );
    
    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ads.retrieve(resultWorkspaceName));
    
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i)
    {
      TS_ASSERT( outputWS->hasMaskedBins(i) );
      const MatrixWorkspace::MaskList& mask = outputWS->maskedBins(i);
      TS_ASSERT_EQUALS( mask.size(), 3 );
      MatrixWorkspace::MaskList::const_iterator it;
      int k = 20;
      for (it = mask.begin(); it != mask.end(); ++it,++k)
      {
        TS_ASSERT_EQUALS( (*it).first, k );
        TS_ASSERT_EQUALS( (*it).second, 1.0 );
      }
      
      for (size_t j = 0; j < outputWS->blocksize(); ++j)
      {
        if ( j >= 20 && j < 23 )
        {
          TS_ASSERT_EQUALS( outputWS->readY(i)[j], 0.0 );
          TS_ASSERT_EQUALS( outputWS->readE(i)[j], 0.0 );
        }
        else
        {
          TS_ASSERT_EQUALS( outputWS->readY(i)[j], 2.0 );
          TS_ASSERT_DELTA( outputWS->readE(i)[j], sqrt(2.0), 0.0001 );
        }
        TS_ASSERT_EQUALS( outputWS->readX(i)[j], j )     ;
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
    
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("InputWorkspace",workspaceName) );
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("OutputWorkspace",workspaceName) );
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("XMin","-11.0") );
    TS_ASSERT_THROWS_NOTHING( masker2.setPropertyValue("XMax","-8.5") );
    
    TS_ASSERT_THROWS_NOTHING( masker2.execute() );
    TS_ASSERT( masker2.isExecuted() );
    
    TS_ASSERT( ! WS->hasMaskedBins(0) );
    TS_ASSERT( WS->hasMaskedBins(1) );
    TS_ASSERT( ! WS->hasMaskedBins(2) );
    
    const MatrixWorkspace::MaskList& mask = WS->maskedBins(1);
    TS_ASSERT_EQUALS( mask.size(), 2 );
    const Mantid::MantidVec& Y = WS->readY(1);
    const Mantid::MantidVec& E = WS->readE(1);
    MatrixWorkspace::MaskList::const_iterator it;
    int k = 0;
    for (it = mask.begin(); it != mask.end(); ++it,++k)
    {
      TS_ASSERT_EQUALS( (*it).first, k );
      TS_ASSERT_EQUALS( (*it).second, 1.0 );
      TS_ASSERT_EQUALS( Y[k], 0.0 );
      TS_ASSERT_EQUALS( E[k], 0.0 );
    }
       
    AnalysisDataService::Instance().remove(workspaceName);
  }
  
  

  void testSpectraList_out_of_range()
  {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(10,10,0.0);
    AnalysisDataService::Instance().add(workspaceName,WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace",workspaceName);
    masker2.setPropertyValue("OutputWorkspace",workspaceName);
    masker2.setPropertyValue("XMin","-11.0");
    masker2.setPropertyValue("XMax","-8.5");
    masker2.setPropertyValue("SpectraList","1,8-12");

    TS_ASSERT_THROWS_NOTHING( masker2.execute() );
    TS_ASSERT( !masker2.isExecuted() );
    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testSpectraList_WS2D()
  {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(5,nBins,0.0);
    AnalysisDataService::Instance().add(workspaceName,WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace",workspaceName);
    masker2.setPropertyValue("OutputWorkspace",workspaceName);
    masker2.setPropertyValue("XMin","3.0");
    masker2.setPropertyValue("XMax","6.0");
    masker2.setPropertyValue("SpectraList","1-3");

    TS_ASSERT_THROWS_NOTHING( masker2.execute() );
    TS_ASSERT( masker2.isExecuted() );

    for (int wi=1; wi<=3; wi++)
      for (int bin=3; bin<6;bin++)
      {
        //std::cout << wi << ":" << bin << "\n";
        TS_ASSERT_EQUALS( WS->dataY(wi)[bin], 0.0 );
      }

    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testEventWorkspace_SpectraList()
  {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS = WorkspaceCreationHelper::CreateEventWorkspace(numHist, nBins) ;
    AnalysisDataService::Instance().add(workspaceName,WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace",workspaceName);
    masker2.setPropertyValue("OutputWorkspace",workspaceName);
    masker2.setPropertyValue("XMin","3.0");
    masker2.setPropertyValue("XMax","6.0");
    masker2.setPropertyValue("SpectraList","1-3");

    TS_ASSERT_THROWS_NOTHING( masker2.execute() );
    TS_ASSERT( masker2.isExecuted() );

    EventWorkspace_const_sptr constWS = boost::dynamic_pointer_cast<const EventWorkspace>(WS);
    for (int wi=1; wi<=3; wi++)
      for (int bin=3; bin<6;bin++)
      {
        //std::cout << wi << ":" << bin << "\n";
        const MantidVec & Y = constWS->dataY(wi);
        TS_ASSERT_EQUALS( Y[bin], 0.0 );
      }

    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testEventWorkspace_No_SpectraList()
  {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS = WorkspaceCreationHelper::CreateEventWorkspace(numHist, nBins) ;
    AnalysisDataService::Instance().add(workspaceName,WS);
    std::size_t events_before = WS->getNumberEvents();

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace",workspaceName);
    masker2.setPropertyValue("OutputWorkspace",workspaceName);
    masker2.setPropertyValue("XMin","3.0");
    masker2.setPropertyValue("XMax","6.0");
    masker2.setPropertyValue("SpectraList",""); //Do all

    TS_ASSERT_THROWS_NOTHING( masker2.execute() );
    TS_ASSERT( masker2.isExecuted() );

    EventWorkspace_const_sptr constWS = boost::dynamic_pointer_cast<const EventWorkspace>(WS);
    std::size_t events_after = constWS->getNumberEvents();

    for (int wi=0; wi<numHist; wi++)
      for (int bin=3; bin<6;bin++)
      {
        //std::cout << wi << ":" << bin << "\n";
        const MantidVec & Y = constWS->dataY(wi);
        TS_ASSERT_EQUALS( Y[bin], 0.0 );
      }
    //Fewer events now; I won't go through all of them
    TS_ASSERT_LESS_THAN(events_after, events_before);

    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testEventWorkspace_copiedOutput_No_SpectraList()
  {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS = WorkspaceCreationHelper::CreateEventWorkspace(numHist, nBins) ;
    AnalysisDataService::Instance().add(workspaceName,WS);
    std::size_t events_before = WS->getNumberEvents();

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace",workspaceName);
    masker2.setPropertyValue("OutputWorkspace",workspaceName+"2");
    masker2.setPropertyValue("XMin","3.0");
    masker2.setPropertyValue("XMax","6.0");
    masker2.setPropertyValue("SpectraList",""); //Do all

    TS_ASSERT_THROWS_NOTHING( masker2.execute() );
    TS_ASSERT( masker2.isExecuted() );

    EventWorkspace_const_sptr constWS = boost::dynamic_pointer_cast<const EventWorkspace>(AnalysisDataService::Instance().retrieve(workspaceName+"2"));
    std::size_t events_after = constWS->getNumberEvents();

    for (int wi=0; wi<numHist; wi++)
      for (int bin=3; bin<6;bin++)
      {
        //std::cout << wi << ":" << bin << "\n";
        const MantidVec & Y = constWS->dataY(wi);
        TS_ASSERT_EQUALS( Y[bin], 0.0 );
      }
    //Fewer events now; I won't go through all of them
    TS_ASSERT_LESS_THAN(events_after, events_before);

    AnalysisDataService::Instance().remove(workspaceName);
    AnalysisDataService::Instance().remove(workspaceName+"2");
  }




private:
  Mantid::Algorithms::MaskBins masker;
};

#endif /*MASKBINSTEST_H_*/
