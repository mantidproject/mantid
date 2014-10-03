#ifndef LOADFITSTEST_H_
#define LOADFITSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadFITS.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class LoadFITSTest : public CxxTest::TestSuite
{
public: 
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
    
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    outputSpace="LoadFITSTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);     
    
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);
    
    inputFile = "FITS_small_01.fits,FITS_small_02.fits";
    algToBeTested.setPropertyValue("Filename", inputFile);  
  }    
 
  void testPerformAssertions()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );
    //  get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    TS_ASSERT_EQUALS( output->blocksize(), 2);  // Number of time bins should equal number of files
    TS_ASSERT_EQUALS( output->getNumberHistograms(), SPECTRA_COUNT);  // Number of spectra
    // Sum the two bins from the last spectra - should be 70400
    double sumY = output->readY(SPECTRA_COUNT-1)[0] + output->readY(SPECTRA_COUNT-1)[1];   
    TS_ASSERT_EQUALS(sumY, 70400);   
    // Check the sum of the error values for the last spectra in each file - should be 375.183
    double sumE = output->readE(SPECTRA_COUNT-1)[0] + output->readE(SPECTRA_COUNT-1)[1];
    TS_ASSERT_LESS_THAN(abs(sumE-375.1830), 0.0001);  // Include a small tolerance check with the assert - not exactly 375.183
  }

  void testSingleChunkSize()
  {
    algToBeTested.setPropertyValue("FileChunkSize", "1");
    testPerformAssertions();
  }

private:
  LoadFITS algToBeTested;
  std::string inputFile;
  std::string outputSpace;
  const static size_t SPECTRA_COUNT = 262144;
};


#endif