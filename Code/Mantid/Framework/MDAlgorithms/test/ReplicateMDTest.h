#ifndef MANTID_MDALGORITHMS_REPLICATEMDTEST_H_
#define MANTID_MDALGORITHMS_REPLICATEMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidMDAlgorithms/ReplicateMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <string>
#include <vector>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class ReplicateMDTest : public CxxTest::TestSuite {

private:

  IMDHistoWorkspace_sptr makeHistoWorkspace(const std::vector<int> &shape) {

    IAlgorithm *create =
        FrameworkManager::Instance().createAlgorithm("CreateMDHistoWorkspace");
    create->setChild(true);
    create->initialize();

    const std::string allNames[5] = {"A", "B", "C", "D", "E"};
    const std::string allUnits[5] = {"AU", "BU", "CU", "DU", "EU"};

    std::vector<std::string> names;
    std::vector<std::string> units;
    size_t flatSize = 1;
    std::vector<double> extents;
    for (size_t i = 0; i < shape.size(); ++i) {
      flatSize *= shape[i];
      names.push_back(allNames[i]);
      units.push_back(allUnits[i]);
      extents.push_back(-10);
      extents.push_back(10);
    }

    create->setProperty("SignalInput", std::vector<double>(flatSize, 1));
    create->setProperty("ErrorInput", std::vector<double>(flatSize, 1));

    create->setProperty("Dimensionality", int(shape.size()));
    create->setProperty("Extents", extents);
    create->setProperty("NumberOfBins", shape);
    create->setProperty("Names", names);
    create->setProperty("Units", units);
    create->setPropertyValue("OutputWorkspace", "dummy");
    create->execute();
    IMDHistoWorkspace_sptr outWs = create->getProperty("OutputWorkspace");
    return outWs;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReplicateMDTest *createSuite() { return new ReplicateMDTest(); }
  static void destroySuite(ReplicateMDTest *suite) { delete suite; }

  void test_init() {
    ReplicateMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_size_check_on_dimensionality() {

    std::vector<int> badDataShape;
    badDataShape.push_back(3);
    badDataShape.push_back(3);
    badDataShape.push_back(3); // 3rd dimension given

    std::vector<int> goodDataShape;
    goodDataShape.push_back(3);
    goodDataShape.push_back(3);
    goodDataShape.push_back(1); // Integrate so should be OK

    std::vector<int> shapeShape;
    shapeShape.push_back(3);
    shapeShape.push_back(3);
    shapeShape.push_back(3);


    auto dataWSGood = makeHistoWorkspace(goodDataShape);
    auto dataWSBad = makeHistoWorkspace(badDataShape);
    auto shapeWS = makeHistoWorkspace(shapeShape);

    ReplicateMD alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWSBad);
    alg.setProperty("ShapeWorkspace", shapeWS);
    TSM_ASSERT_EQUALS("Shape and data are the same size. Should fail.", 1,
                      alg.validateInputs().size());

    // Try again with different property value
    alg.setProperty("DataWorkspace", dataWSGood);
    TSM_ASSERT_EQUALS("Interated dim should not be counted.", 0,
                      alg.validateInputs().size());
  }

  void test_basic_shape_check() {
    auto shapeWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 3 /*numDims*/, 4 /*numBins in each dimension*/);

    // Data workspace is right size (number of dimensions), but wrong shape
    // (number of bins in each)
    auto dataWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, shapeWS->getNumDims() - 1 /*numDims*/,
        3 /*numBins in each dimension*/);

    ReplicateMD alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("DataWorkspace", dataWS);
    alg.setProperty("ShapeWorkspace", shapeWS);
    TSM_ASSERT_EQUALS("Shape and data are different shapes. Should fail.", 1,
                      alg.validateInputs().size());
  }

  void test_very_simple_exec() {
      auto shapeWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
          1 /*signal*/, 3 /*numDims*/, 4 /*numBins in each dimension*/);


      auto dataWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
          2 /*signal*/, 2 /*numDims*/,
          4 /*numBins in each dimension*/);

      ReplicateMD alg;
      alg.setRethrows(true);
      alg.setChild(true);
      alg.initialize();
      alg.setProperty("DataWorkspace", dataWS);
      alg.setProperty("ShapeWorkspace", shapeWS);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

      // Very basic sanity checks
      TS_ASSERT(outWS);
      TS_ASSERT_EQUALS(shapeWS->getNumDims(), outWS->getNumDims());
      TS_ASSERT_EQUALS(shapeWS->getNPoints(), outWS->getNPoints());
      TS_ASSERT_EQUALS(dataWS->getSignalAt(0), outWS->getSignalAt(0));
  }
};

#endif /* MANTID_MDALGORITHMS_REPLICATEMDTEST_H_ */
