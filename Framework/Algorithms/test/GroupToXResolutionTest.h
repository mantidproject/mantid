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

using namespace Mantid;

class GroupToXResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupToXResolutionTest *createSuite() { return new GroupToXResolutionTest(); }
  static void destroySuite( GroupToXResolutionTest *suite ) { delete suite; }


  void test_Init() {
    Algorithms::GroupToXResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    HistogramData::Points Xs{0.23};
    HistogramData::Counts Ys{1.42};
    HistogramData::Histogram h(Xs, Ys);
    API::MatrixWorkspace_sptr inputWS = DataObjects::create<DataObjects::Workspace2D>(1, std::move(h));
    auto Dxs = Kernel::make_cow<HistogramData::HistogramDx>(1, 1.);
    inputWS->setSharedDx(0, std::move(Dxs));
    Algorithms::GroupToXResolution alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
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
};


#endif /* MANTID_ALGORITHMS_GROUPTOXRESOLUTIONTEST_H_ */
