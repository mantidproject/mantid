#ifndef TRANSPOSETEST_H_
#define TRANSPOSETEST_H_

#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/Transpose.h"

#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class TransposeTest : public CxxTest::TestSuite
{

public:
  TransposeTest() {}
  ~TransposeTest() {}

  void testMetaInfo()
  {
    transpose = new Transpose();
    TS_ASSERT_EQUALS(transpose->name(), "Transpose");
    TS_ASSERT_EQUALS(transpose->version(), 1);
    TS_ASSERT_EQUALS(transpose->category(), "General");
    delete transpose;
  }

  void testInit()
  {
    transpose = new Transpose();
    TS_ASSERT_THROWS_NOTHING(transpose->initialize());
    TS_ASSERT(transpose->isInitialized());
    delete transpose;
  }

  void testExec()
  {
    IAlgorithm* loader;
    loader = new Mantid::DataHandling::LoadRaw;
    loader->initialize();
    loader->setPropertyValue("Filename", "../../../../Test/AutoTestData/IRS21360.raw");
    loader->setPropertyValue("OutputWorkspace", "transpose_irs_r");
    loader->setPropertyValue("SpectrumMin", "3");
    loader->setPropertyValue("SpectrumMax", "13");

    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    delete loader;

    transpose = new Transpose();

    if ( !transpose->isInitialized() )
    {
      transpose->initialize();
    }

    // Input workspace
    MatrixWorkspace_const_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("transpose_irs_r"));

    const int nHist = inputWS->getNumberHistograms();
    const int nBins = inputWS->blocksize();

    TS_ASSERT_THROWS(transpose->execute(), std::runtime_error);
    TS_ASSERT(!transpose->isExecuted());

    TS_ASSERT_THROWS_NOTHING(transpose->setPropertyValue("InputWorkspace", "transpose_irs_r"));
    TS_ASSERT_THROWS_NOTHING(transpose->setPropertyValue("OutputWorkspace", "transpose_irs_t"));
    TS_ASSERT_THROWS_NOTHING(transpose->execute());
    TS_ASSERT(transpose->isExecuted());

    // Get output workspace
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("transpose_irs_t")));

    // Dimensions
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(), outputWS->blocksize());
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), inputWS->blocksize());

    // Units
    TS_ASSERT_EQUALS(inputWS->getAxis(0)->unit(), outputWS->getAxis(1)->unit());
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit(), inputWS->getAxis(1)->unit());

    // Values
    TS_ASSERT_EQUALS(inputWS->readY(0)[0], outputWS->readY(0)[0]);
    TS_ASSERT_EQUALS(inputWS->readY(nHist-1)[nBins-1], outputWS->readY(nBins-1)[nHist-1]);

    delete transpose;
  }
private:
  Transpose* transpose;

};
#endif
