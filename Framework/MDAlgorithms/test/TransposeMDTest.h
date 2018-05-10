#ifndef MANTID_MDALGORITHMS_TRANSPOSEMDTEST_H_
#define MANTID_MDALGORITHMS_TRANSPOSEMDTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDAlgorithms/TransposeMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using Mantid::MDAlgorithms::TransposeMD;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class TransposeMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TransposeMDTest *createSuite() { return new TransposeMDTest(); }
  static void destroySuite(TransposeMDTest *suite) { delete suite; }

  void test_Init() {
    TransposeMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_valid_axes_lower_limit_throws() {
    TransposeMD transposeMD;
    transposeMD.initialize();
    std::vector<int> axes;
    axes.push_back(1); // should be fine.
    TS_ASSERT_THROWS_NOTHING(transposeMD.setProperty("Axes", axes));
    axes.push_back(-1); // Not a valid axis
    TS_ASSERT_THROWS(transposeMD.setProperty("Axes", axes),
                     std::invalid_argument &);
  }

  void test_too_many_dimension_indexes_throws() {
    auto inputWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

    TransposeMD transposeMD;
    transposeMD.setChild(true);
    transposeMD.initialize();
    transposeMD.setPropertyValue("OutputWorkspace", "dummy");
    transposeMD.setProperty(
        "Axes",
        std::vector<int>(4, 1)); // 4-axis entries, but only 3 dimensions
    transposeMD.setProperty("InputWorkspace", inputWS);
    TS_ASSERT_THROWS(transposeMD.execute(), std::invalid_argument &);
  }

  void test_indexes_that_dont_exist_throws() {
    auto inputWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

    TransposeMD transposeMD;
    transposeMD.setChild(true);
    transposeMD.initialize();
    transposeMD.setPropertyValue("OutputWorkspace", "dummy");
    transposeMD.setProperty("Axes",
                            std::vector<int>(1, 3)); // Invalid index of 3!
    transposeMD.setProperty("InputWorkspace", inputWS);
    TSM_ASSERT_THROWS(
        "Axis values can only be 0-2 for this ws. 3 is not valid.",
        transposeMD.execute(), std::invalid_argument &);
  }

  void test_no_transpose() {
    auto inputWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

    // Set some values. If transposed then these should end up elsewhere.
    inputWS->setSignalAt(0, 2);
    inputWS->setSignalAt(1, 2);

    TransposeMD transposeMD;
    transposeMD.setChild(true);
    transposeMD.initialize();
    transposeMD.setPropertyValue("OutputWorkspace", "dummy");
    transposeMD.setProperty("InputWorkspace", inputWS);
    transposeMD.execute();
    IMDHistoWorkspace_sptr outputWS =
        transposeMD.getProperty("OutputWorkspace");

    // Lets check that the workspaces are essentially the same.
    TS_ASSERT_EQUALS(inputWS->getNumDims(), outputWS->getNumDims());
    TS_ASSERT_EQUALS(inputWS->getDimension(0)->getName(),
                     outputWS->getDimension(0)->getName());
    TS_ASSERT_EQUALS(inputWS->getDimension(1)->getName(),
                     outputWS->getDimension(1)->getName());

    // Data should be the same too.
    TS_ASSERT_EQUALS(inputWS->getSignalAt(0), outputWS->getSignalAt(0));
    TS_ASSERT_EQUALS(inputWS->getSignalAt(1), outputWS->getSignalAt(1));
    TS_ASSERT_EQUALS(inputWS->getSignalAt(2), outputWS->getSignalAt(2));
  }

  void test_transpose_all() {
    auto inputWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

    // Set some values. If transposed then these should end up elsewhere.
    inputWS->setSignalAt(0, 2);
    inputWS->setSignalAt(1, 2);

    TransposeMD transposeMD;
    transposeMD.setChild(true);
    transposeMD.initialize();
    transposeMD.setPropertyValue("OutputWorkspace", "dummy");
    transposeMD.setProperty("InputWorkspace", inputWS);
    std::vector<int> axes;
    axes.push_back(1);
    axes.push_back(0);
    transposeMD.setProperty("Axes", axes);
    transposeMD.execute();
    IMDHistoWorkspace_sptr outputWS =
        transposeMD.getProperty("OutputWorkspace");

    // Lets check the output workspace
    TS_ASSERT_EQUALS(inputWS->getNumDims(), outputWS->getNumDims());
    TS_ASSERT_EQUALS(inputWS->getDimension(0)->getName(),
                     outputWS->getDimension(axes[0])->getName());
    TS_ASSERT_EQUALS(inputWS->getDimension(1)->getName(),
                     outputWS->getDimension(axes[1])->getName());

    // Data should be transposed.
    TS_ASSERT_EQUALS(inputWS->getSignalAt(0), outputWS->getSignalAt(0));
    TS_ASSERT_EQUALS(inputWS->getSignalAt(1), outputWS->getSignalAt(1 * 3));
    TS_ASSERT_EQUALS(inputWS->getSignalAt(2), outputWS->getSignalAt(2));
  }

  void test_collapse() {

    size_t nbins[3] = {3, 3, 1}; // last dimension integrated out
    Mantid::coord_t min[3] = {0, 0, 0};
    Mantid::coord_t max[3] = {10, 10, 5};
    auto inputWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        3 /*ndims*/, 1 /*signal*/, 1 /*errorSquared*/, nbins /*numBins*/, min,
        max);
    // Set some values. If transposed then these should end up elsewhere.
    inputWS->setSignalAt(0, 2);
    inputWS->setSignalAt(1, 2);

    TransposeMD transposeMD;
    transposeMD.setChild(true);
    transposeMD.initialize();
    transposeMD.setPropertyValue("OutputWorkspace", "dummy");
    transposeMD.setProperty("InputWorkspace", inputWS);
    std::vector<int> axes;
    axes.push_back(0);
    axes.push_back(1);
    transposeMD.setProperty("Axes", axes); // 0 and 1, but 2 not specified!
    transposeMD.execute();
    IMDHistoWorkspace_sptr outputWS =
        transposeMD.getProperty("OutputWorkspace");

    // Lets check that output workspace
    TS_ASSERT_EQUALS(inputWS->getNumDims(), outputWS->getNumDims() + 1);
    TS_ASSERT_EQUALS(inputWS->getDimension(0)->getName(),
                     outputWS->getDimension(axes[0])->getName());
    TS_ASSERT_EQUALS(inputWS->getDimension(1)->getName(),
                     outputWS->getDimension(axes[1])->getName());

    // Otherwise the data should be the same. We simply clipped off the
    // integrated dimension.
    TS_ASSERT_EQUALS(inputWS->getSignalAt(0), outputWS->getSignalAt(0));
    TS_ASSERT_EQUALS(inputWS->getSignalAt(1), outputWS->getSignalAt(1));
    TS_ASSERT_EQUALS(inputWS->getSignalAt(2), outputWS->getSignalAt(2));
  }
};

#endif /* MANTID_MDALGORITHMS_TRANSPOSEMDTEST_H_ */
