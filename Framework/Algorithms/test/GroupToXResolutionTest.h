// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GROUPTOXRESOLUTIONTEST_H_
#define MANTID_ALGORITHMS_GROUPTOXRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GroupToXResolution.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/QuadraticGenerator.h"

#include <boost/math/special_functions/pow.hpp>

using namespace Mantid;
using boost::math::pow;

class GroupToXResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupToXResolutionTest *createSuite() {
    return new GroupToXResolutionTest();
  }
  static void destroySuite(GroupToXResolutionTest *suite) { delete suite; }

  void test_Init() {
    Algorithms::GroupToXResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_single_point_remains_unchanged() {
    HistogramData::Points Xs{0.23};
    HistogramData::Counts Ys{1.42};
    HistogramData::Histogram h(Xs, Ys);
    API::MatrixWorkspace_sptr inputWS =
        DataObjects::create<DataObjects::Workspace2D>(1, std::move(h));
    auto Dxs = Kernel::make_cow<HistogramData::HistogramDx>(1, 1.);
    inputWS->setSharedDx(0, std::move(Dxs));
    Algorithms::GroupToXResolution alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->x(0).front(), 0.23)
    TS_ASSERT_EQUALS(outputWS->y(0).front(), 1.42)
    TS_ASSERT_EQUALS(outputWS->e(0).front(), std::sqrt(1.42))
    TS_ASSERT(outputWS->hasDx(0))
    TS_ASSERT_EQUALS(outputWS->dx(0).front(), 1.)
  }

  void test_two_points_get_averaged() {
    HistogramData::Points Xs{0.2, 0.6};
    HistogramData::Counts Ys{1.5, 2.5};
    HistogramData::CountStandardDeviations Es{2., 3.};
    HistogramData::Histogram h(Xs, Ys, Es);
    API::MatrixWorkspace_sptr inputWS =
        DataObjects::create<DataObjects::Workspace2D>(1, std::move(h));
    auto Dxs = Kernel::make_cow<HistogramData::HistogramDx>(2);
    {
      auto &DxData = Dxs.access();
      DxData.front() = 1.2;
      DxData.back() = 1.7;
    }
    inputWS->setSharedDx(0, std::move(Dxs));
    Algorithms::GroupToXResolution alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FractionOfDx", 1.))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->x(0).front(), (0.2 + 0.6) / 2.)
    TS_ASSERT_EQUALS(outputWS->y(0).front(), (1.5 + 2.5) / 2.)
    TS_ASSERT_EQUALS(outputWS->e(0).front(),
                     std::sqrt(pow<2>(2.) + pow<2>(3.)) / 2.)
    TS_ASSERT(outputWS->hasDx(0))
    TS_ASSERT_EQUALS(outputWS->dx(0).front(),
                     std::sqrt(pow<2>(1.2) + pow<2>(0.68 * (0.6 - 0.2))))
  }

  void test_two_spearate_points_remain_unchanged() {
    HistogramData::Points Xs{0.2, 0.6};
    HistogramData::Counts Ys{1.5, 2.5};
    HistogramData::CountStandardDeviations Es{2., 3.};
    HistogramData::Histogram h(Xs, Ys, Es);
    API::MatrixWorkspace_sptr inputWS =
        DataObjects::create<DataObjects::Workspace2D>(1, std::move(h));
    auto Dxs = Kernel::make_cow<HistogramData::HistogramDx>(2);
    {
      auto &DxData = Dxs.access();
      DxData.front() = 0.1;
      DxData.back() = 0.3;
    }
    inputWS->setSharedDx(0, std::move(Dxs));
    Algorithms::GroupToXResolution alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FractionOfDx", 1.))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 2)
    TS_ASSERT_EQUALS(outputWS->x(0).front(), 0.2)
    TS_ASSERT_EQUALS(outputWS->y(0).front(), 1.5)
    TS_ASSERT_EQUALS(outputWS->e(0).front(), 2.)
    TS_ASSERT(outputWS->hasDx(0))
    TS_ASSERT_EQUALS(outputWS->dx(0).front(), 0.1)
    TS_ASSERT_EQUALS(outputWS->x(0).back(), 0.6)
    TS_ASSERT_EQUALS(outputWS->y(0).back(), 2.5)
    TS_ASSERT_EQUALS(outputWS->e(0).back(), 3.)
    TS_ASSERT_EQUALS(outputWS->dx(0).back(), 0.3)
  }

  void test_four_points_grouped_into_two() {
    HistogramData::Points Xs{0.2, 0.6, 5.1, 5.7};
    HistogramData::Counts Ys{1.5, 2.5, -2.5, -1.5};
    HistogramData::CountStandardDeviations Es{2., 3., 2.5, 1.5};
    HistogramData::Histogram h(Xs, Ys, Es);
    API::MatrixWorkspace_sptr inputWS =
        DataObjects::create<DataObjects::Workspace2D>(1, std::move(h));
    auto Dxs = Kernel::make_cow<HistogramData::HistogramDx>(4);
    {
      auto &DxData = Dxs.access();
      DxData = {1., 0.1, 2., 0.2};
    }
    inputWS->setSharedDx(0, std::move(Dxs));
    Algorithms::GroupToXResolution alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FractionOfDx", 1.))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 2)
    TS_ASSERT_EQUALS(outputWS->x(0).front(), (0.2 + 0.6) / 2.)
    TS_ASSERT_EQUALS(outputWS->y(0).front(), (1.5 + 2.5) / 2.)
    TS_ASSERT_EQUALS(outputWS->e(0).front(),
                     std::sqrt(pow<2>(2.) + pow<2>(3.)) / 2.)
    TS_ASSERT(outputWS->hasDx(0))
    TS_ASSERT_EQUALS(outputWS->dx(0).front(),
                     std::sqrt(pow<2>(1.) + pow<2>(0.68 * (0.6 - 0.2))))
    TS_ASSERT_EQUALS(outputWS->x(0).back(), (5.1 + 5.7) / 2.)
    TS_ASSERT_EQUALS(outputWS->y(0).back(), (-2.5 + -1.5) / 2)
    TS_ASSERT_EQUALS(outputWS->e(0).back(),
                     std::sqrt(pow<2>(2.5) + pow<2>(1.5)) / 2.)
    TS_ASSERT_EQUALS(outputWS->dx(0).back(),
                     std::sqrt(pow<2>(2.) + pow<2>(0.68 * (5.7 - 5.1))))
  }
};

class GroupToXResolutionTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupToXResolutionTestPerformance *createSuite() {
    return new GroupToXResolutionTestPerformance();
  }
  static void destroySuite(GroupToXResolutionTestPerformance *suite) {
    delete suite;
  }

  GroupToXResolutionTestPerformance() : m_alg() {
    m_alg.setRethrows(true);
    m_alg.setChild(true);
    m_alg.initialize();
  }

  void setUp() override {
    constexpr double xZeroth{0.};
    constexpr double xFirst{0.};
    constexpr double xSecond{0.4};
    constexpr size_t n{10000};
    HistogramData::Points Xs(
        n, HistogramData::QuadraticGenerator(xZeroth, xFirst, xSecond));
    HistogramData::Counts Ys(n, 1.3);
    HistogramData::CountStandardDeviations Es(n, 1.1);
    HistogramData::Histogram h(Xs, Ys, Es);
    API::MatrixWorkspace_sptr inputWS =
        DataObjects::create<DataObjects::Workspace2D>(1, std::move(h));
    // Construct DX such that in the beginning, we group multiple points
    // and after a crossover, no grouping happens.
    constexpr double initialGroupSize{10.};
    constexpr double crossover{0.8 * n};
    constexpr double dxZeroth{2. * initialGroupSize * xSecond};
    constexpr double dxFirst{(2. * crossover - 2. * initialGroupSize + 1.) /
                             crossover * xSecond};
    auto Dxs = Kernel::make_cow<HistogramData::HistogramDx>(
        n, HistogramData::LinearGenerator(dxZeroth, dxFirst));
    inputWS->setSharedDx(0, std::move(Dxs));
    m_alg.setProperty("InputWorkspace", inputWS);
    m_alg.setProperty("OutputWorkspace", "_out");
    m_alg.setProperty("FractionOfDx", 1.);
  }

  void test_performance() {
    for (int i = 0; i < 5000; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute())
    }
  }

private:
  Algorithms::GroupToXResolution m_alg;
};

#endif /* MANTID_ALGORITHMS_GROUPTOXRESOLUTIONTEST_H_ */
