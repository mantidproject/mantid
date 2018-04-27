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

#include <array>

using Mantid::DataHandling::JoinISISPolarizationEfficiencies;

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

  void test_fileIsReadCorrectly() {
  }
};

#endif /* MANTID_DATAHANDLING_JOINISISPOLARIZATIONEFFICIENCIESTEST_H_ */
