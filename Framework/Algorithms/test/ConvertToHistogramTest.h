// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/ConvertToHistogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVecPtr;
using Mantid::Algorithms::ConvertToHistogram;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramDx;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;
using Mantid::Kernel::make_cow;

class ConvertToHistogramTest : public CxxTest::TestSuite {

public:
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void test_That_The_Algorithm_Has_Two_Properties() {
    ConvertToHistogram alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.getProperties().size(), 2);
  }

  void test_That_Output_Is_The_Same_As_Input_If_Input_Contains_Histogram_Data() {
    // True indicates a non histogram workspace
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspace123(5, 10, true);

    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    // Check that the algorithm just pointed the output data at the input
    TS_ASSERT_EQUALS(&(*testWS), &(*outputWS));
  }

  void test_A_Point_Data_InputWorkspace_Is_Converted_To_A_Histogram() {
    // Creates a workspace with 10 points
    const int numYPoints(10);
    const int numSpectra(2);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspace123(numSpectra, numYPoints, false);
    // Reset the X data to something reasonable
    Points x(numYPoints, LinearGenerator(0.0, 1.0));
    for (int i = 0; i < numSpectra; ++i) {
      testWS->setPoints(i, x);
    }

    TS_ASSERT_EQUALS(testWS->isHistogramData(), false);

    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    TS_ASSERT_EQUALS(outputWS->isHistogramData(), true);
    const int numBoundaries = numYPoints + 1;

    const double expectedX[11] = {-0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5};
    for (int j = 0; j < numBoundaries; ++j) {
      TS_ASSERT_EQUALS(outputWS->readX(0)[j], expectedX[j]);
    }
  }

  void test_Dx_Data_Is_Handled_Correctly() {
    // Creates a workspace with 10 points
    constexpr int numYPoints{10};
    constexpr int numSpectra{2};
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspace123(numSpectra, numYPoints, false);
    double xErrors[numYPoints] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    auto dxs = make_cow<HistogramDx>(xErrors, xErrors + numYPoints);
    // Reset the X data to something reasonable, set Dx.
    Points x(numYPoints, LinearGenerator(0.0, 1.0));
    for (int i = 0; i < numSpectra; ++i) {
      testWS->setPoints(i, x);
      testWS->setSharedDx(i, dxs);
    }
    TS_ASSERT(!testWS->isHistogramData())
    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);
    TS_ASSERT(outputWS);
    TS_ASSERT(outputWS->isHistogramData())
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      TS_ASSERT(outputWS->hasDx(i))
      const auto &dx = outputWS->dx(i);
      TS_ASSERT_EQUALS(dx.size(), numYPoints)
      for (size_t j = 0; j < dx.size(); ++j) {
        TS_ASSERT_EQUALS(dx[j], xErrors[j])
      }
    }
  }

  void test_ragged() {
    // create ragged workspace
    Workspace2D_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspace(2, 1);

    // create and replace histograms with ragged ones
    raggedWS->setHistogram(0, Histogram(Points{150., 250., 350.}, Counts{1., 2., 3.}));
    raggedWS->setHistogram(1, Histogram(Points{300., 500.}, Counts{4., 5.}));

    // quick check of the input workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    TS_ASSERT(!raggedWS->isHistogramData())
    TS_ASSERT_EQUALS(raggedWS->getNumberHistograms(), 2);

    MatrixWorkspace_sptr outputWS = runAlgorithm(raggedWS);
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isHistogramData())   // output is a histogram workspace
    TS_ASSERT(outputWS->isRaggedWorkspace()) // output is a ragged workspace
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 2);

    // check the data
    const Mantid::MantidVec &Y0 = outputWS->readY(0);
    const Mantid::MantidVec &X0 = outputWS->readX(0);
    const Mantid::MantidVec &Y1 = outputWS->readY(1);
    const Mantid::MantidVec &X1 = outputWS->readX(1);
    TS_ASSERT_EQUALS(Y0.size(), 3);
    TS_ASSERT_EQUALS(X0.size(), 4);
    TS_ASSERT_EQUALS(Y1.size(), 2);
    TS_ASSERT_EQUALS(X1.size(), 3);
    TS_ASSERT_EQUALS(X0[0], 100.);
    TS_ASSERT_EQUALS(X0[1], 200.);
    TS_ASSERT_EQUALS(X0[2], 300.);
    TS_ASSERT_EQUALS(X0[3], 400.);
    TS_ASSERT_EQUALS(X1[0], 200.);
    TS_ASSERT_EQUALS(X1[1], 400.);
    TS_ASSERT_EQUALS(X1[2], 600.);
    TS_ASSERT_EQUALS(Y0[0], 1.);
    TS_ASSERT_EQUALS(Y0[1], 2.);
    TS_ASSERT_EQUALS(Y0[2], 3.);
    TS_ASSERT_EQUALS(Y1[0], 4.);
    TS_ASSERT_EQUALS(Y1[1], 5.);
  }

private:
  MatrixWorkspace_sptr runAlgorithm(const Workspace2D_sptr &inputWS) {
    IAlgorithm_sptr alg(new ConvertToHistogram());
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS));
    const std::string outputName = "__algOut";
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", outputName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr outputWS =
        std::dynamic_pointer_cast<MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outputName));

    return outputWS;
  }
};

class ConvertToHistogramTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToHistogramTestPerformance *createSuite() { return new ConvertToHistogramTestPerformance(); }

  static void destroySuite(ConvertToHistogramTestPerformance *suite) { delete suite; }

  void setUp() override { inputWS = WorkspaceCreationHelper::create2DWorkspace123(20000, 10000, false); }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove("output"); }

  void testPerformanceWS() {
    ConvertToHistogram cth;
    cth.initialize();
    cth.setProperty("InputWorkspace", inputWS);
    cth.setProperty("OutputWorkspace", "output");
    cth.execute();
  }

private:
  Workspace2D_sptr inputWS;
};
