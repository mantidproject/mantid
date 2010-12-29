#ifndef TOFCORRECTIONTEST_H_
#define TOFCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/TofCorrection.h"

#include "MantidDataHandling/LoadRaw3.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class TofCorrectionTest : public CxxTest::TestSuite
{

public:
  TofCorrectionTest() {}
  ~TofCorrectionTest() {}

  void testMetaInfo()
  {
    tof = new TofCorrection();
    TS_ASSERT_EQUALS(tof->name(), "TofCorrection");
    TS_ASSERT_EQUALS(tof->version(), 1);
    TS_ASSERT_EQUALS(tof->category(), "General");
    delete tof;
  }

  void testInit()
  {
    tof = new TofCorrection();
    TS_ASSERT_THROWS_NOTHING(tof->initialize());
    TS_ASSERT(tof->isInitialized());
    delete tof;
  }

  void testExec()
  {
    IAlgorithm* loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "../../../../Test/AutoTestData/TSC10076.raw");
    loader->setPropertyValue("OutputWorkspace", "tofcorrection_tsc_r");
    loader->setPropertyValue("SpectrumMin", "13");
    loader->setPropertyValue("SpectrumMax", "13");

    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    delete loader;

    tof = new TofCorrection();

    if ( !tof->isInitialized() )
    {
      tof->initialize();
    }

    // Input workspace
    MatrixWorkspace_const_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("tofcorrection_tsc_r"));

    const int nBins = inputWS->blocksize();

    TS_ASSERT_THROWS(tof->execute(), std::runtime_error);
    TS_ASSERT(!tof->isExecuted());

    TS_ASSERT_THROWS_NOTHING(tof->setPropertyValue("InputWorkspace", "tofcorrection_tsc_r"));
    TS_ASSERT_THROWS_NOTHING(tof->setPropertyValue("OutputWorkspace", "output"));
    TS_ASSERT_THROWS_NOTHING(tof->execute());
    TS_ASSERT(tof->isExecuted());

    // Get output workspace
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("output")));

    const MantidVec inputDataX = inputWS->readX(0);
    const MantidVec outputDataX = outputWS->readX(0);
    
    // Output workspace should have same dimensions as the input
    TS_ASSERT_EQUALS(inputWS->blocksize(), outputWS->blocksize());
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(), outputWS->getNumberHistograms());

    // Units should also be the same
    TS_ASSERT_EQUALS(inputWS->getAxis(1)->unit(), outputWS->getAxis(1)->unit());
    TS_ASSERT_EQUALS(inputWS->getAxis(0)->unit(), outputWS->getAxis(0)->unit());

    // Check that the bins for each spectra are changed by the same amount
    double differenceA = inputDataX[0] - outputDataX[0];
    double differenceB = inputDataX[nBins] - outputDataX[nBins];
    TS_ASSERT_DELTA(differenceA, differenceB, 0.001);
    // And that they are actually changed
    TS_ASSERT_DIFFERS(inputDataX[0], outputDataX[0]);
    TS_ASSERT_DIFFERS(inputDataX[nBins], outputDataX[nBins]);
    // Check that the Y and E values are the same
    TS_ASSERT_EQUALS(inputWS->readY(0)[0], outputWS->readY(0)[0]);
    TS_ASSERT_EQUALS(inputWS->readY(0)[nBins-1], outputWS->readY(0)[nBins-1]);
    TS_ASSERT_EQUALS(inputWS->readE(0)[0], outputWS->readE(0)[0]);
    TS_ASSERT_EQUALS(inputWS->readE(0)[nBins-1], outputWS->readE(0)[nBins-1]);

    delete tof;
  }
private:
  TofCorrection* tof;

};
#endif
