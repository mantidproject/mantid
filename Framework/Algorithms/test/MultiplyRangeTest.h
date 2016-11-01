#ifndef MULTIPLYRANGETEST_H_
#define MULTIPLYRANGETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/MultiplyRange.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class MultiplyRangeTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(mr.name(), "MultiplyRange"); }

  void testVersion() { TS_ASSERT_EQUALS(mr.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(mr.initialize());
    TS_ASSERT(mr.isInitialized());
  }

  void testExec() {
    if (!mr.isInitialized())
      mr.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "OFFSPEC00004622.raw");
    loader.setPropertyValue("OutputWorkspace", "tomultiply");
    loader.setPropertyValue("SpectrumList", "1");
    loader.setPropertyValue("LoadLogFiles", "0");
    loader.execute();

    TS_ASSERT_THROWS_NOTHING(
        mr.setPropertyValue("InputWorkspace", "tomultiply"));
    TS_ASSERT_THROWS_NOTHING(
        mr.setPropertyValue("OutputWorkspace", "multiplied"));
    TS_ASSERT_THROWS_NOTHING(mr.setPropertyValue("StartBin", "60"));
    TS_ASSERT_THROWS_NOTHING(mr.setPropertyValue("EndBin", "1000"));
    TS_ASSERT_THROWS_NOTHING(mr.setPropertyValue("Factor", "1.25"));

    TS_ASSERT_THROWS_NOTHING(mr.execute());
    TS_ASSERT(mr.isExecuted());

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("tomultiply")));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("multiplied")));

    const size_t length = result->blocksize();
    for (size_t i = 0; i < length; ++i) {
      TS_ASSERT_EQUALS(in->x(0)[i], result->x(0)[i]);
      if (i >= 60 && i <= 1000) {
        TS_ASSERT_EQUALS(in->y(0)[i] * 1.25, result->y(0)[i]);
        TS_ASSERT_EQUALS(in->e(0)[i] * 1.25, result->e(0)[i]);
      } else {
        TS_ASSERT_EQUALS(in->y(0)[i], result->y(0)[i]);
        TS_ASSERT_EQUALS(in->e(0)[i], result->e(0)[i]);
      }
    }

    AnalysisDataService::Instance().remove("tomultiply");
    AnalysisDataService::Instance().remove("multiplied");
  }

private:
  Mantid::Algorithms::MultiplyRange mr;
};

#endif /*MULTIPLYRANGETEST_H_*/
