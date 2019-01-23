// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef WEIGHTEDMEANTEST_H_
#define WEIGHTEDMEANTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/WeightedMean.h"
#include "MantidDataHandling/LoadRaw3.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class WeightedMeanTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(wm.name(), "WeightedMean") }

  void testVersion() { TS_ASSERT_EQUALS(wm.version(), 1) }

  void testCategory() { TS_ASSERT_EQUALS(wm.category(), "Arithmetic") }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(wm.initialize())
    TS_ASSERT(wm.isInitialized())
  }

  void testExec() {
    if (!wm.isInitialized())
      wm.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "OFFSPEC00004622.raw");
    loader.setPropertyValue("OutputWorkspace", "first");
    loader.setPropertyValue("SpectrumList", "1");
    loader.setPropertyValue("LoadLogFiles", "0");
    loader.execute();

    Mantid::DataHandling::LoadRaw3 loader2;
    loader2.initialize();
    loader2.setPropertyValue("Filename", "OFFSPEC00004622.raw");
    loader2.setPropertyValue("OutputWorkspace", "second");
    loader2.setPropertyValue("SpectrumList", "2");
    loader2.setPropertyValue("LoadLogFiles", "0");
    loader2.execute();

    TS_ASSERT_THROWS_NOTHING(wm.setPropertyValue("InputWorkspace1", "first"))
    TS_ASSERT_THROWS_NOTHING(wm.setPropertyValue("InputWorkspace2", "second"))
    TS_ASSERT_THROWS_NOTHING(wm.setPropertyValue("OutputWorkspace", "result"))

    TS_ASSERT_THROWS_NOTHING(wm.execute())
    TS_ASSERT(wm.isExecuted())

    MatrixWorkspace_const_sptr in2, result;
    TS_ASSERT_THROWS_NOTHING(
        in2 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("second")))
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("result")))
    // Check bin boundaries are the same
    TS_ASSERT_EQUALS(in2->x(0), result->x(0))
    // Pick a bin where both entries are non-zero
    TS_ASSERT_DELTA(result->y(0)[1176], 21983.40535, 0.00001)
    TS_ASSERT_DELTA(result->e(0)[1176], 104.841321, 0.000001)
    // Now one where first is zero
    TS_ASSERT_EQUALS(result->y(0)[2], 2.0)
    TS_ASSERT_EQUALS(result->e(0)[2], M_SQRT2)
    // And one where second is zero
    TS_ASSERT_EQUALS(result->y(0)[113], 97.0)
    TS_ASSERT_EQUALS(result->e(0)[113], std::sqrt(97.0))
    // Finally one where both are zero
    TS_ASSERT_EQUALS(result->y(0)[4989], 0.0)
    TS_ASSERT_EQUALS(result->e(0)[4989], 0.0)

    AnalysisDataService::Instance().remove("first");
    AnalysisDataService::Instance().remove("second");
    AnalysisDataService::Instance().remove("result");
  }

private:
  Mantid::Algorithms::WeightedMean wm;
};

#endif /*WEIGHTEDMEANTEST_H_*/
