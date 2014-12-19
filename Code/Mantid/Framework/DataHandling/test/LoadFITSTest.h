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

    // Set the ImageKey to be 0 (as it is missing from the test file and is required);
    algToBeTested.setProperty<int>("ImageKey", 0);
  }    
 
  void testPerformAssertions()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );
    //  get workspace generated
    WorkspaceGroup_sptr output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);   
    TS_ASSERT_EQUALS( output->getNumberOfEntries(), 2);  // Number of time bins should equal number of files
    MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(output->getItem(0));
    MatrixWorkspace_sptr ws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(output->getItem(1));
    
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), SPECTRA_COUNT);  // Number of spectra
    // Sum the two bins from the last spectra - should be 70400
    double sumY = ws1->readY(SPECTRA_COUNT-1)[0] + ws2->readY(SPECTRA_COUNT-1)[0];   
    TS_ASSERT_EQUALS(sumY, 275);   
    // Check the sum of the error values for the last spectra in each file - should be 375.183
    double sumE = ws1->readE(SPECTRA_COUNT-1)[0] + ws2->readE(SPECTRA_COUNT-1)[0];
    TS_ASSERT_LESS_THAN(abs(sumE-23.4489), 0.0001);  // Include a small tolerance check with the assert - not exactly 375.183
  }

private:
  LoadFITS algToBeTested;
  std::string inputFile;
  std::string outputSpace;
  const static size_t SPECTRA_COUNT = 262144; // Based on the 512*512 test image
};


#endif