// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidReflectometry/LoadILLPolarizationFactors.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"

#include <array>

using Mantid::Reflectometry::LoadILLPolarizationFactors;

class LoadILLPolarizationFactorsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLPolarizationFactorsTest *createSuite() {
    return new LoadILLPolarizationFactorsTest();
  }
  static void destroySuite(LoadILLPolarizationFactorsTest *suite) {
    delete suite;
  }

  void test_initialization() {
    LoadILLPolarizationFactors alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_fileIsReadCorrectly() {
    using namespace Mantid::API;
    using namespace Mantid::HistogramData;
    using namespace Mantid::DataObjects;
    const BinEdges edges{0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const Counts counts(edges.size() - 1, 0.0);
    const Histogram h{edges, counts};
    auto refWS = MatrixWorkspace_sptr{create<Workspace2D>(1, h).release()};

    LoadILLPolarizationFactors alg;
    alg.setRethrows(true);
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Filename", "ILL/D17/PolarizationFactors.txt"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "LoadILLPolarizationFactorsTest"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WavelengthReference", refWS))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    std::set<std::string> factorTags{"F1", "F2", "P1", "P2", "Phi"};
    Mantid::API::MatrixWorkspace_sptr outWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), factorTags.size())
    const auto vertAxis = outWS->getAxis(1);
    TS_ASSERT(vertAxis)
    TS_ASSERT(vertAxis->isText());
    TS_ASSERT_EQUALS(vertAxis->length(), factorTags.size())
    const auto points = Points(edges).rawData();
    for (size_t i = 0; i != 5; ++i) {
      const auto label = vertAxis->label(i);
      const auto tagIter = factorTags.find(label);
      TS_ASSERT_DIFFERS(tagIter, factorTags.end())
      std::vector<double> fs;
      double errorFactor;
      if (*tagIter == "F1") {
        fs = factors(points, F1_limits(), F1_K());
        errorFactor = 1. / 3000.;
      } else if (*tagIter == "F2") {
        fs = factors(points, F2_limits(), F2_K());
        errorFactor = 1. / 3000.;
      } else if (*tagIter == "P1") {
        fs = factors(points, P1_limits(), P1_K());
        errorFactor = 1. / 500.;
      } else if (*tagIter == "P2") {
        fs = factors(points, P2_limits(), P2_K());
        errorFactor = 1. / 500.;
      } else {
        fs = factors(points, Phi_limits(), Phi_K());
        errorFactor = 1. / 500.;
      }
      const auto &xs = outWS->x(i);
      TS_ASSERT_EQUALS(xs.size(), edges.size())
      const auto &ys = outWS->y(i);
      TS_ASSERT_EQUALS(ys.size(), counts.size())
      const auto &es = outWS->e(i);
      TS_ASSERT_EQUALS(es.size(), counts.size())
      for (size_t j = 0; j != ys.size(); ++j) {
        TS_ASSERT_EQUALS(xs[j], edges[j])
        TS_ASSERT_DELTA(ys[j], fs[j], 1e-12)
        TS_ASSERT_EQUALS(es[j], errorFactor * ys[j])
      }
      factorTags.erase(tagIter);
    }
  }

private:
  static std::array<double, 4> F1_limits() {
    // Values directly from the test file.
    return {{6.0000, 8.0000, 10.0000, 14.0000}};
  }

  static std::array<double, 6> F1_K() {
    // Values directly from the test file.
    return {{0.9950, -0.0004, -0.0014, -0.0007, -0.0011, -0.0023}};
  }

  static std::array<double, 4> F2_limits() {
    // Values directly from the test file.
    return {{6.0000, 8.5000, 12.0000, 16.0000}};
  }

  static std::array<double, 6> F2_K() {
    // Values directly from the test file.
    return {{0.9918, 0.0000, -0.0003, -0.0011, -0.0011, -0.0011}};
  }

  static std::array<double, 4> P1_limits() {
    // Values directly from the test file.
    return {{7.0120, 7.4048, 14.2916, 16.3}};
  }

  static std::array<double, 6> P1_K() {
    // Values directly from the test file.
    return {{-0.0002, 0.0006, -0.0023, 0.0001, 0.0043, -0.000}};
  }

  static std::array<double, 4> P2_limits() {
    // Values directly from the test file.
    return {{6.7983, 11.0000, 14.0000, 16.85}};
  }

  static std::array<double, 6> P2_K() {
    // Values directly from the test file.
    return {{0.0136, -0.0014, 0.0020, 0.0030, 0.0088, 0.0178}};
  }

  static std::array<double, 4> Phi_limits() {
    // Values directly from the test file.
    return {{6.6115, 8.3926, 9.5390, 13.8787}};
  }

  static std::array<double, 6> Phi_K() {
    // Values directly from the test file.
    return {{0.0114, -0.0005, 0.0007, 0.0019, 0.0027, 0.0120}};
  }

  static std::vector<double> factors(const std::vector<double> &wavelength,
                                     const std::array<double, 4> &limits,
                                     const std::array<double, 6> &K) {
    // Adaptation of the IDL code from the LAMP/COSMOS software.
    std::array<double, 5> A;
    A[0] = K[0];
    A[1] = A[0] + K[1] * limits[0];
    A[2] = A[1] + K[2] * (limits[1] - limits[0]);
    A[3] = A[2] + K[3] * (limits[2] - limits[1]);
    A[4] = A[3] + K[4] * (limits[3] - limits[2]);
    std::vector<double> L(wavelength.size());
    auto l1 =
        std::upper_bound(wavelength.cbegin(), wavelength.cend(), limits[0]);
    size_t b = 0;
    size_t e = std::distance(wavelength.cbegin(), l1);
    for (size_t i = b; i != e; ++i) {
      L[i] = A[0] + K[1] * wavelength[i];
    }
    auto l2 = std::upper_bound(l1 - 1, wavelength.cend(), limits[1]);
    b = e;
    e = b + std::distance(l1, l2);
    for (size_t i = b; i != e; ++i) {
      L[i] = A[1] + K[2] * (wavelength[i] - limits[0]);
    }
    l1 = l2;
    l2 = std::upper_bound(l1 - 1, wavelength.cend(), limits[2]);
    b = e;
    e = b + std::distance(l1, l2);
    for (size_t i = b; i != e; ++i) {
      L[i] = A[2] + K[3] * (wavelength[i] - limits[1]);
    }
    l1 = l2;
    l2 = std::upper_bound(l1 - 1, wavelength.cend(), limits[3]);
    b = e;
    e = b + std::distance(l1, l2);
    for (size_t i = b; i != e; ++i) {
      L[i] = A[3] + K[4] * (wavelength[i] - limits[2]);
    }
    b = e;
    e = wavelength.size();
    for (size_t i = b; i != e; ++i) {
      L[i] = A[4] + K[5] * (wavelength[i] - limits[3]);
    }
    return L;
  }
};

class LoadILLPolarizationFactorsTestPerformance : public CxxTest::TestSuite {
public:
  void test_loadingLargeHistogram() {
    const size_t nBins = 1000000;
    const std::vector<double> y(nBins, 1.0);
    Mantid::HistogramData::Counts counts(y);
    std::vector<double> x(nBins + 1);
    std::iota(x.begin(), x.end(), 0.0);
    const Mantid::HistogramData::BinEdges edges(x);
    const Mantid::HistogramData::Histogram h(edges, counts);
    Mantid::API::MatrixWorkspace_sptr ws =
        Mantid::DataObjects::create<Mantid::DataObjects::Workspace2D>(1, h);
    for (size_t i = 0; i < 100; ++i) {
      LoadILLPolarizationFactors alg;
      alg.setRethrows(true);
      alg.setChild(true);
      alg.initialize();
      alg.setProperty("Filename", "ILL/D17/PolarizationFactors.txt");
      alg.setProperty("OutputWorkspace", "LoadILLPolarizationFactorsTest");
      alg.setProperty("WavelengthReference", ws);
      alg.execute();
    }
  }
};
