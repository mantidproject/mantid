#ifndef MANTID_ALGORITHMS_EXTRACTUNMASKEDSPECTRATEST_H_
#define MANTID_ALGORITHMS_EXTRACTUNMASKEDSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/ExtractUnmaskedSpectra.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <algorithm>

using Mantid::Algorithms::ExtractUnmaskedSpectra;

class ExtractUnmaskedSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractUnmaskedSpectraTest *createSuite() {
    return new ExtractUnmaskedSpectraTest();
  }
  static void destroySuite(ExtractUnmaskedSpectraTest *suite) { delete suite; }

  void test_Init() {
    ExtractUnmaskedSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Something() { 
    
  }

private:
  Mantid::API::MatrixWorkspace_sptr createInputWorkspace(bool isMasked = true) {
    auto workspace = WorkspaceCreationHelper::Create2DWorkspace(10, 3);
    if (isMasked) {
      std::vector<size_t> indices(5);
      {
        size_t i = 0;
        std::generate(indices.begin(), indices.end(), [&i]() { return 2 * i; });
      }
      auto alg =
          Mantid::API::AlgorithmFactory::Instance().create("MaskDetectors", -1);
      alg->setChild(true);
      alg->setProperty("InputWorkspace", workspace);
      alg->setProperty("WorkspaceIndexList", indices);
      alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
      alg->execute();
      workspace = alg->getProperty("OutputWorkspace");
    }
    return workspace;
  }

  Mantid::API::MatrixWorkspace_sptr
  runAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWS) {
    ExtractUnmaskedSpectra alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Mantid::API::MatrixWorkspace_sptr outputWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    return outputWS;
  }
};

#endif /* MANTID_ALGORITHMS_EXTRACTUNMASKEDSPECTRATEST_H_ */