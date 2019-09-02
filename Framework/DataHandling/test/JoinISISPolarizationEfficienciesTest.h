// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIESTEST_H_
#define MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/JoinISISPolarizationEfficiencies.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/Unit.h"

#include <array>
#include <numeric>

using Mantid::DataHandling::JoinISISPolarizationEfficiencies;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

class JoinISISPolarizationEfficienciesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JoinISISPolarizationEfficienciesTest *createSuite() {
    return new JoinISISPolarizationEfficienciesTest();
  }
  static void destroySuite(JoinISISPolarizationEfficienciesTest *suite) {
    delete suite;
  }

  void test_initialization() {
    JoinISISPolarizationEfficiencies alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_no_input() {
    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    // Error: At least one of the efficiency file names must be set.
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

  void test_mixed_input() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(10, 0, 10);
    auto ws3 = createHistoWS(10, 0, 10);
    auto ws4 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Pp", ws1);
    alg.setProperty("Ap", ws2);
    alg.setProperty("P1", ws3);
    alg.setProperty("P2", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    // Error: Efficiencies belonging to different methods cannot mix.
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

  void test_fredrikze() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(10, 0, 10);
    auto ws3 = createHistoWS(10, 0, 10);
    auto ws4 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Pp", ws1);
    alg.setProperty("Ap", ws2);
    alg.setProperty("Rho", ws3);
    alg.setProperty("Alpha", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "Pp");
    TS_ASSERT_EQUALS(axis1->label(1), "Ap");
    TS_ASSERT_EQUALS(axis1->label(2), "Rho");
    TS_ASSERT_EQUALS(axis1->label(3), "Alpha");

    TS_ASSERT(outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(2);
      auto const &y = outWS->y(2);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(3);
      auto const &y = outWS->y(3);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }
  }

  void test_wildes() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(10, 0, 10);
    auto ws3 = createHistoWS(10, 0, 10);
    auto ws4 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("P1", ws1);
    alg.setProperty("P2", ws2);
    alg.setProperty("F1", ws3);
    alg.setProperty("F2", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");
    TS_ASSERT_EQUALS(axis1->label(2), "F1");
    TS_ASSERT_EQUALS(axis1->label(3), "F2");

    TS_ASSERT(outWS->isHistogramData());
  }

  void test_wildes_points() {
    auto ws1 = createPointWS(10, 0, 10);
    auto ws2 = createPointWS(10, 0, 10);
    auto ws3 = createPointWS(10, 0, 10);
    auto ws4 = createPointWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("P1", ws1);
    alg.setProperty("P2", ws2);
    alg.setProperty("F1", ws3);
    alg.setProperty("F2", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");
    TS_ASSERT_EQUALS(axis1->label(2), "F1");
    TS_ASSERT_EQUALS(axis1->label(3), "F2");

    TS_ASSERT(!outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 10);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
      auto sum = std::accumulate(y.begin(), y.end(), 0.0);
      TS_ASSERT_DELTA(sum, 10.0, 1e-14);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 10);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
      auto sum = std::accumulate(y.begin(), y.end(), 0.0);
      TS_ASSERT_DELTA(sum, 10.0, 1e-14);
    }

    {
      auto const &x = outWS->x(2);
      auto const &y = outWS->y(2);
      TS_ASSERT_EQUALS(x.size(), 10);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
      auto sum = std::accumulate(y.begin(), y.end(), 0.0);
      TS_ASSERT_DELTA(sum, 10.0, 1e-14);
    }

    {
      auto const &x = outWS->x(3);
      auto const &y = outWS->y(3);
      TS_ASSERT_EQUALS(x.size(), 10);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
      auto sum = std::accumulate(y.begin(), y.end(), 0.0);
      TS_ASSERT_DELTA(sum, 10.0, 1e-14);
    }
  }

  void test_histo_3_out_of_4() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(10, 0, 10);
    auto ws3 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("P1", ws1);
    alg.setProperty("P2", ws2);
    alg.setProperty("F1", ws3);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "P2");
    TS_ASSERT_EQUALS(axis1->label(2), "F1");
  }

  void test_histo_2_out_of_4() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("P1", ws1);
    alg.setProperty("F1", ws2);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "P1");
    TS_ASSERT_EQUALS(axis1->label(1), "F1");
  }

  void test_histo_1_out_of_4() {
    auto ws1 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("F2", ws1);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "F2");
  }

  void test_mixed_histo_points() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createPointWS(10, 0, 10);
    auto ws3 = createHistoWS(10, 0, 10);
    auto ws4 = createHistoWS(10, 0, 10);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("P1", ws1);
    alg.setProperty("P2", ws2);
    alg.setProperty("F1", ws3);
    alg.setProperty("F2", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    // Error: Cannot mix histograms and point data.
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

  void test_ragged() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(10, 1, 10);
    auto ws3 = createHistoWS(10, 2, 3);
    auto ws4 = createHistoWS(10, 11, 20);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Pp", ws1);
    alg.setProperty("Ap", ws2);
    alg.setProperty("Rho", ws3);
    alg.setProperty("Alpha", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 10);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "Pp");
    TS_ASSERT_EQUALS(axis1->label(1), "Ap");
    TS_ASSERT_EQUALS(axis1->label(2), "Rho");
    TS_ASSERT_EQUALS(axis1->label(3), "Alpha");

    TS_ASSERT(outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 0);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 1);
      TS_ASSERT_EQUALS(x.back(), 10);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(2);
      auto const &y = outWS->y(2);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 2);
      TS_ASSERT_EQUALS(x.back(), 3);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(3);
      auto const &y = outWS->y(3);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 10);
      TS_ASSERT_EQUALS(x.front(), 11);
      TS_ASSERT_EQUALS(x.back(), 20);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }
  }

  void test_histo_ragged_diff_sizes() {
    auto ws1 = createHistoWS(10, 0, 10);
    auto ws2 = createHistoWS(9, 1, 10);
    auto ws3 = createHistoWS(11, 2, 3);
    auto ws4 = createHistoWS(10, 11, 20);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Pp", ws1);
    alg.setProperty("Ap", ws2);
    alg.setProperty("Rho", ws3);
    alg.setProperty("Alpha", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 11);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "Pp");
    TS_ASSERT_EQUALS(axis1->label(1), "Ap");
    TS_ASSERT_EQUALS(axis1->label(2), "Rho");
    TS_ASSERT_EQUALS(axis1->label(3), "Alpha");

    TS_ASSERT(outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 12);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 0., 1e-15);
      TS_ASSERT_DELTA(x.back(), 10, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 12);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 1., 1e-15);
      TS_ASSERT_DELTA(x.back(), 10, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }

    {
      auto const &x = outWS->x(2);
      auto const &y = outWS->y(2);
      TS_ASSERT_EQUALS(x.size(), 12);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 2.0, 1e-9);
      TS_ASSERT_DELTA(x.back(), 3.0, 1e-9);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(3);
      auto const &y = outWS->y(3);
      TS_ASSERT_EQUALS(x.size(), 12);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 11.0, 1e-15);
      TS_ASSERT_DELTA(x.back(), 20.0, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1., 1e-15);
      TS_ASSERT_DELTA(y.back(), 1., 1e-15);
    }
  }

  void test_points_ragged_diff_sizes() {
    auto ws1 = createPointWS(10, 0, 10);
    auto ws2 = createPointWS(9, 1, 10);
    auto ws3 = createPointWS(11, 2, 3);
    auto ws4 = createPointWS(10, 11, 20);

    JoinISISPolarizationEfficiencies alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Pp", ws1);
    alg.setProperty("Ap", ws2);
    alg.setProperty("Rho", ws3);
    alg.setProperty("Alpha", ws4);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(outWS->blocksize(), 11);
    TS_ASSERT_EQUALS(outWS->getAxis(0)->unit()->caption(), "Wavelength");

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "Pp");
    TS_ASSERT_EQUALS(axis1->label(1), "Ap");
    TS_ASSERT_EQUALS(axis1->label(2), "Rho");
    TS_ASSERT_EQUALS(axis1->label(3), "Alpha");

    TS_ASSERT(!outWS->isHistogramData());

    {
      auto const &x = outWS->x(0);
      auto const &y = outWS->y(0);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 0, 1e-5);
      TS_ASSERT_DELTA(x.back(), 10.0, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1.0, 1e-15);
      TS_ASSERT_DELTA(y.back(), 1.0, 1e-15);
    }

    {
      auto const &x = outWS->x(1);
      auto const &y = outWS->y(1);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 1.0, 1e-15);
      TS_ASSERT_DELTA(x.back(), 10.0, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1.0, 1e-15);
      TS_ASSERT_DELTA(y.back(), 1.0, 1e-15);
    }

    {
      auto const &x = outWS->x(2);
      auto const &y = outWS->y(2);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_EQUALS(x.front(), 2);
      TS_ASSERT_EQUALS(x.back(), 3);
      TS_ASSERT_EQUALS(y.front(), 1);
      TS_ASSERT_EQUALS(y.back(), 1);
    }

    {
      auto const &x = outWS->x(3);
      auto const &y = outWS->y(3);
      TS_ASSERT_EQUALS(x.size(), 11);
      TS_ASSERT_EQUALS(y.size(), 11);
      TS_ASSERT_DELTA(x.front(), 11.0, 1e-15);
      TS_ASSERT_DELTA(x.back(), 20.0, 1e-15);
      TS_ASSERT_DELTA(y.front(), 1.0, 1e-15);
      TS_ASSERT_DELTA(y.back(), 1.0, 1e-15);
    }
  }

private:
  MatrixWorkspace_sptr createHistoWS(size_t size, double startX,
                                     double endX) const {
    double const dX = (endX - startX) / double(size);
    BinEdges xVals(size + 1, LinearGenerator(startX, dX));
    Counts yVals(size, 1.0);
    auto retVal = boost::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }

  MatrixWorkspace_sptr createPointWS(size_t size, double startX,
                                     double endX) const {
    double const dX = (endX - startX) / double(size - 1);
    Points xVals(size, LinearGenerator(startX, dX));
    Counts yVals(size, 1.0);
    auto retVal = boost::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }
};

#endif /* MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIESTEST_H_ */
