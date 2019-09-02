// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TRANSPOSETEST_H_
#define TRANSPOSETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/Transpose.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/RebinnedOutput.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class TransposeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TransposeTest *createSuite() { return new TransposeTest(); }
  static void destroySuite(TransposeTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void testMetaInfo() {
    transpose = new Transpose();
    TS_ASSERT_EQUALS(transpose->name(), "Transpose");
    TS_ASSERT_EQUALS(transpose->version(), 1);
    delete transpose;
  }

  void testInit() {
    transpose = new Transpose();
    TS_ASSERT_THROWS_NOTHING(transpose->initialize());
    TS_ASSERT(transpose->isInitialized());
    delete transpose;
  }

  void testExec() {
    IAlgorithm *loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "IRS21360.raw");
    loader->setPropertyValue("OutputWorkspace", "transpose_irs_r");
    loader->setPropertyValue("SpectrumMin", "3");
    loader->setPropertyValue("SpectrumMax", "13");

    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    delete loader;

    transpose = new Transpose();

    if (!transpose->isInitialized()) {
      transpose->initialize();
    }

    // Input workspace
    MatrixWorkspace_const_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "transpose_irs_r");

    const size_t nHist = inputWS->getNumberHistograms();
    const size_t nBins = inputWS->blocksize();

    TS_ASSERT_THROWS(transpose->execute(), const std::runtime_error &);
    TS_ASSERT(!transpose->isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        transpose->setPropertyValue("InputWorkspace", "transpose_irs_r"));
    TS_ASSERT_THROWS_NOTHING(
        transpose->setPropertyValue("OutputWorkspace", "transpose_irs_t"));
    TS_ASSERT_THROWS_NOTHING(transpose->execute());
    TS_ASSERT(transpose->isExecuted());

    // Get output workspace
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "transpose_irs_t"));

    // Dimensions
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(), outputWS->blocksize());
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), inputWS->blocksize());

    // Units
    TS_ASSERT_EQUALS(inputWS->getAxis(0)->unit(), outputWS->getAxis(1)->unit());
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit(), inputWS->getAxis(1)->unit());

    // Values
    TS_ASSERT_EQUALS(inputWS->y(0)[0], outputWS->y(0)[0]);
    TS_ASSERT_EQUALS(inputWS->y(nHist - 1)[nBins - 1],
                     outputWS->y(nBins - 1)[nHist - 1]);

    delete transpose;
  }

  void testRebinnedOutput() {
    RebinnedOutput_sptr inputWS =
        WorkspaceCreationHelper::createRebinnedOutputWorkspace();
    std::string inName = inputWS->getName();
    AnalysisDataService::Instance().addOrReplace(inName, inputWS);
    std::string outName = "rebinTrans";
    transpose = new Transpose();
    if (!transpose->isInitialized()) {
      transpose->initialize();
    }
    transpose->setPropertyValue("InputWorkspace", inName);
    transpose->setPropertyValue("OutputWorkspace", outName);
    TS_ASSERT_THROWS_NOTHING(transpose->execute());
    TS_ASSERT(transpose->isExecuted());

    RebinnedOutput_sptr outputWS;
    outputWS =
        AnalysisDataService::Instance().retrieveWS<RebinnedOutput>(outName);
    TS_ASSERT(outputWS);
    // Dimensions
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(), outputWS->blocksize());
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), inputWS->blocksize());

    // Value
    TS_ASSERT_EQUALS(outputWS->mutableY(3)[1], inputWS->mutableY(1)[3]);
    TS_ASSERT_DELTA(outputWS->mutableE(3)[1], inputWS->mutableE(1)[3], 1.e-5);
    TS_ASSERT_EQUALS(outputWS->dataF(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->dataF(3)[1], inputWS->dataF(1)[3]);
    // Check a nan
    bool inNan = boost::math::isnan(inputWS->mutableY(0)[5]);
    bool outNan = boost::math::isnan(outputWS->mutableY(5)[0]);
    TS_ASSERT_EQUALS(outNan, inNan);

    delete transpose;
  }

private:
  Transpose *transpose;
};

class TransposeTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TransposeTestPerformance *createSuite() {
    return new TransposeTestPerformance();
  }
  static void destroySuite(TransposeTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  TransposeTestPerformance() {

    // set up testExecPerformance
    IAlgorithm *loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "IRS21360.raw");
    loader->setPropertyValue("OutputWorkspace", "transpose_irs_r");
    loader->setPropertyValue("SpectrumMin", "3");
    loader->setPropertyValue("SpectrumMax", "13");

    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    delete loader;

    // set up testRebinnedOutputPerformance
    RebinnedOutput_sptr inputWS =
        WorkspaceCreationHelper::createRebinnedOutputWorkspace();
    std::string inName = rebinned_inputWS;
    AnalysisDataService::Instance().addOrReplace(inName, inputWS);
  }

  void testExecPerformance() {
    // Input workspace
    Transpose transpose;
    MatrixWorkspace_const_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "transpose_irs_r");
    transpose.initialize();

    transpose.setPropertyValue("InputWorkspace", "transpose_irs_r");
    transpose.setPropertyValue("OutputWorkspace", "transpose_irs_t");
    TS_ASSERT_THROWS_NOTHING(transpose.execute());
    TS_ASSERT(transpose.isExecuted());
  }

  void testRebinnedOutputPerformance() {
    Transpose transpose;
    transpose.initialize();
    transpose.setPropertyValue("InputWorkspace", rebinned_inputWS);
    transpose.setPropertyValue("OutputWorkspace", rebinned_outputWS);
    TS_ASSERT_THROWS_NOTHING(transpose.execute());
    TS_ASSERT(transpose.isExecuted());
  }

  const std::string rebinned_inputWS = "rebinned_inputWS";
  const std::string rebinned_outputWS = "rebinned_outputWS";
};
#endif
