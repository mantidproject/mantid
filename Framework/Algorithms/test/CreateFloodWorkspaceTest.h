#ifndef MANTID_ALGORITHMS_CREATEFLOODWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATEFLOODWORKSPACETEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateFloodWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace WorkspaceCreationHelper;

class CreateFloodWorkspaceTest : public CxxTest::TestSuite {
public:

  void test_Init() {
    CreateFloodWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_no_inputs() {
    CreateFloodWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error &e, std::string(e.what()), "Some invalid Properties found");
  }

  void test_inconsistent_inputs() {
    auto ws = create1DWorkspaceRand(1, 1);
    storeWS("ws", ws);
    CreateFloodWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", "OFFSPEC00004622.raw");
    alg.setPropertyValue("InputWorkspace", "ws");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error &e, std::string(e.what()), "Some invalid Properties found");
    removeWS("ws");
  }

  void test_create() {
    auto ws = createWS();
    CreateFloodWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.execute();
    MatrixWorkspace_sptr flood = alg.getProperty("OutputWorkspace");
    TS_ASSERT(flood);
  }

private:

  MatrixWorkspace_sptr createWS() {
    static std::vector<double> const random{
        0.95696224, 0.78608634, 1.02309468, 0.92736103, 0.96011047, 1.0827529,
        1.06064806, 0.86867923, 0.86722594, 0.92285179, 0.95882377, 0.88258063,
        1.10531192, 0.96573216, 1.02895327, 1.01548801, 0.9719391,
        1.0477047,  0.88327841, 1.09285156, 0.94490405, 1.10175312, 1.02961563,
        1.26504126, 0.99778468, 0.90924367, 1.13339998, 1.09677771, 0.90571331,
        0.99389186};

    auto ws = create2DWorkspaceFromFunction([](double x, int i){
      return random[i] * (10.0 + 0.1 * x);
    }, int(random.size()), 0, 10, 1, true);
    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_CREATEFLOODWORKSPACETEST_H_ */
