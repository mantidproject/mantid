// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef EXTRACTFFTSPECTRUM_H_
#define EXTRACTFFTSPECTRUM_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ExtractFFTSpectrum.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ExtractFFTSpectrumTest : public CxxTest::TestSuite {

public:
  void testMetaInfo() {
    ExtractFFTSpectrum alg;
    TS_ASSERT_EQUALS(alg.name(), "ExtractFFTSpectrum");
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testInit() {
    ExtractFFTSpectrum alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    Mantid::DataHandling::LoadNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "IRS26176_ipg.nxs");
    loader.setPropertyValue("OutputWorkspace", "alg_irs_r");
    loader.setPropertyValue("SpectrumMin", "2");
    loader.setPropertyValue("SpectrumMax", "3");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "alg_irs_r");
    rebin.setPropertyValue("OutputWorkspace", "alg_irs_r");
    rebin.setPropertyValue("Params", "-0.5,0.005,0.5");
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "alg_irs_r");

    ExtractFFTSpectrum alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.execute(),
                     const std::runtime_error &); // check it does output error
    TS_ASSERT(!alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "alg_irs_r"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputImagWorkspace",
        "alg_irs_r")); // use same spectra for the imaginary part (Re==Im)
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "alg_irs_t"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get output workspace
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "alg_irs_t"));

    // Dimensions
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(),
                     outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(inputWS->blocksize(), outputWS->blocksize());

    // Units ( Axis 1 should be the same, Axis 0 should be "Time/ns"
    TS_ASSERT(*inputWS->getAxis(1)->unit() == *outputWS->getAxis(1)->unit());
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->caption(), "Time");
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->label(), "ns");
  }
};
#endif // EXTRACTFFTSPECTRUM_H
