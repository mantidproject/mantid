#ifndef EXTRACTSINGLESPECTRUMTEST_H_
#define EXTRACTSINGLESPECTRUMTEST_H_

#include "CropWorkspaceTest.h" // Use the test lable functionality as it should do the same thing
#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::detid_t;

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
    
    const int nbins(5);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(5,nbins);

    const int wsIndex = 2;
    for (int i=0; i<nbins+1; ++i)
    {
      inputWS->dataX(wsIndex)[i] = i;
      if( i < nbins )
      {
        inputWS->dataY(wsIndex)[i] = 20-i;
        inputWS->dataE(wsIndex)[i] = 7;
      }
    }
    inputWS->getAxis(1)->spectraNo(wsIndex) = wsIndex;
    AnalysisDataService::Instance().add("input",inputWS);
    
    TS_ASSERT_THROWS_NOTHING( extractor.setPropertyValue("InputWorkspace","input") )
    TS_ASSERT_THROWS_NOTHING( extractor.setPropertyValue("OutputWorkspace","output") )
    TS_ASSERT_THROWS_NOTHING( extractor.setProperty("WorkspaceIndex",wsIndex) )
    
    TS_ASSERT_THROWS_NOTHING( extractor.execute() )
    TS_ASSERT( extractor.isExecuted() )
    
    Workspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("output"); )
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT( outputWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(output) )
    TS_ASSERT_EQUALS( outputWS->blocksize(), 5 )
    TS_ASSERT_EQUALS( outputWS->readX(0).size(), nbins+1)
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), wsIndex )
    for (int j=0; j<nbins+1; ++j)
    {
      TS_ASSERT_EQUALS( outputWS->readX(0)[j], j );
      if( j < nbins )
      {
        TS_ASSERT_EQUALS( outputWS->readY(0)[j], 20-j );
        TS_ASSERT_EQUALS( outputWS->readE(0)[j], 7 );
      }
    }

    const Mantid::API::ISpectrum *spectrum(NULL);
    TS_ASSERT_THROWS_NOTHING(spectrum = outputWS->getSpectrum(0));
    if( spectrum )
    {
      std::set<detid_t> detids = spectrum->getDetectorIDs();
      TS_ASSERT_EQUALS(detids.size(), 1);
      const detid_t id = *(detids.begin());
      TS_ASSERT_EQUALS(id, 3);
    }
    else
    {
      TS_FAIL("No spectra/detectors associated with extracted histogram.");
    }
    
    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("output");
  }

  void test_Input_With_TextAxis()
  {
    Algorithm *extractor = new ExtractSingleSpectrum;
    extractor->initialize();
    extractor->setPropertyValue("WorkspaceIndex", "1");
    CropWorkspaceTest::doTestWithTextAxis(extractor); //Takes ownership

  }
  
private:
  Mantid::Algorithms::ExtractSingleSpectrum extractor; 
};

#endif /*EXTRACTSINGLESPECTRUMTEST_H_*/
