#ifndef GETQSINQENSDATATEST_H_
#define GETQSINQENSDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/DeleteWorkspace.h"
#include "MantidAlgorithms/GetQsInQENSData.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class GetQsInQENSDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetQsInQENSDataTest *createSuite() {
    return new GetQsInQENSDataTest();
  }
  static void destroySuite(GetQsInQENSDataTest *suite) { delete suite; }

  /*
   * Tests initializing the GetQsInQENSData algorithm
   */
  void testInit() {
    GetQsInQENSData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /*
   * Tests if the correct error message is generated from executing the
   * GetQsInQENSData algorithm with an input workspace without detectors
   */
  void testNoDetectors() {
    const std::string outputWsName = "data";
    const std::string verticalAxisUnit = "Label";
    const std::vector<double> dataX = {0, 1};
    const std::vector<double> dataY = {0, 0};
    const std::vector<double> verticalAxisValues = {0, 1};
    const size_t numSpectra = 2;

    // Create the input workspace, without detectors
    CreateWorkspace createAlg;
    createAlg.setProperty("OutputWorkspace", outputWsName);
    createAlg.setProperty("DataX", dataX);
    createAlg.setProperty("DataY", dataY);
    createAlg.setProperty("NSpec", numSpectra);
    createAlg.setProperty("VerticalAxisUnit", verticalAxisUnit);
    createAlg.setProperty("VerticalAxisValues", verticalAxisValues);
    createAlg.execute();

    MatrixWorkspace_sptr workspace = createAlg.getProperty("OutputWorkspace");
    const std::string expectedErrorMsg =
        "Detectors are missing from the input workspace";

    try {
      GetQsInQENSData alg;
      alg.setProperty("InputWorkspace", workspace);
      alg.setProperty("RaiseMode", true);
      alg.execute();
    } catch (std::exception &e) {
      std::string errorMsg = e.what();
      TS_ASSERT(errorMsg.find(expectedErrorMsg) != std::string::npos);
    }

    TS_ASSERT(false);
  }
};

#endif /* GETQSINQENSDATATEST_H_ */
