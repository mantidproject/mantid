// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REGROUPTEST_H_
#define REGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/Regroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::LinearGenerator;

class RegroupTest : public CxxTest::TestSuite {
public:
  void testworkspace1D_dist() {
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Regroup regroup;
    regroup.initialize();
    regroup.setChild(true);
    regroup.setPropertyValue("InputWorkspace", "test_in1D");
    regroup.setPropertyValue("OutputWorkspace", "test_out");
    // Check it fails if "params" property not set
    TS_ASSERT_THROWS(regroup.execute(), const std::runtime_error &)
    TS_ASSERT(!regroup.isExecuted())
    // Trying to set the property with an error fails
    TS_ASSERT_THROWS(
        regroup.setPropertyValue("Params", "1.5,2.0,20,-0.1,15,1.0,35"),
        const std::invalid_argument &)
    // Now set the property
    TS_ASSERT_THROWS_NOTHING(
        regroup.setPropertyValue("Params", "1.5,1,19,-0.1,30,1,35"))

    TS_ASSERT(regroup.execute())
    TS_ASSERT(regroup.isExecuted())

    MatrixWorkspace_sptr rebindata = regroup.getProperty("OutputWorkspace");
    auto &outX = rebindata->x(0);

    TS_ASSERT_DELTA(outX[7], 12.5, 0.000001);
    TS_ASSERT_DELTA(outX[12], 20.75, 0.000001);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

private:
  Workspace2D_sptr Create1DWorkspace(int size) {
    auto retVal = createWorkspace<Workspace2D>(1, size, size - 1);
    BinEdges x(size, LinearGenerator(0.5, 0.75));
    Counts y(size - 1, 3.0);

    retVal->setHistogram(0, x, y);
    return retVal;
  }

  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen) {
    BinEdges x1(xlen, LinearGenerator(0.5, 0.75));
    Counts y1(xlen - 1, 3.0);

    auto retVal = createWorkspace<Workspace2D>(ylen, xlen, xlen - 1);

    for (int i = 0; i < ylen; i++) {
      retVal->setHistogram(i, x1, y1);
    }

    return retVal;
  }
};
#endif /* REGROUPTEST */
