#ifndef MANTID_MDALGORITHMS_COMPACTMDTEST_H_
#define MANTID_MDALGORITHMS_COMPACTMDTEST_H_
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using Mantid::MDAlgorithms::CompactMD;
using namespace Mantid::API;

//==================
// Functional Tests
//==================
class CompactMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompactMDTest *createSuite() { return new CompactMDTest(); }
  static void destroySuite(CompactMDTest *suite) { delete suite; }

  void test_Init() {
    CompactMD alg;
    TSM_ASSERT_THROWS_NOTHING("Instance of CompactMD threw: ",
                              alg.initialize());
    TSM_ASSERT("Instance of CompactMD was not initialised: ",
               alg.isInitialized());
  }
  void
  test_all_non_zero_signals_are_kept_with_data_concentrated_in_the_centre() {
    /*
     *testing the effectiveness of CompactMD when the data looks like this:
     *------------------
     * Input structure:
     *------------------
     *  -------------
     *  |   |   |///|   |   |
     *  ---------------------
     * -5-4-3 2-1 0 1 2 3 4 5
     *---------------------------
     * Expected output structure:
     *----------------------------
     * should trim until the first non-zero value.
     *    -----
     *    |///|
     *    -----
     *  -1  0  1
     */

    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 0.0;
    const double errorSquared = 1.3;
    size_t numBins[static_cast<int>(numDims)] = {5};
    Mantid::coord_t min[static_cast<int>(numDims)] = {-5};
    Mantid::coord_t max[static_cast<int>(numDims)] = {5};
    const std::string name("test");
    auto inWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    inWS->setSignalAt(2, 1.0); // set middle bin signal to one
    CompactMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    // output workspace should be cropped so extents ~ [-1,1]
    IMDHistoWorkspace_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS(
        "Should have a signal of 1.0: ", outputWorkspace->getSignalAt(0), 1);
    TSM_ASSERT_EQUALS("Minimum should be cropped to -1: ",
                      outputWorkspace->getDimension(0)->getMinimum(), -1.0);
    TSM_ASSERT_EQUALS("Maximum should be cropped to 1: ",
                      outputWorkspace->getDimension(0)->getMaximum(), 1.0);
    TSM_ASSERT_EQUALS("Number of Bins should be 1 : ",
                      outputWorkspace->getDimension(0)->getNBins(), 1.0);
    TSM_ASSERT_EQUALS("Bin width should be consistent: ",
                      outputWorkspace->getDimension(0)->getBinWidth(),
                      inWS->getDimension(0)->getBinWidth());
  }
  void test_all_non_zero_signals_are_kept_with_data_in_each_corner() {
    /*
     *testing the effectiveness of CompactMD when the data looks like this:
     *-----------------------------------
     * Input structure: 2D HistoWorkspace
     *-----------------------------------
     *  ------------- -3
     *  |/a/|   |/b/| -2
     *  ------------- -1
     *  |   |   |   |  0
     *  -------------  1
     *  |/c/|   |/d/|  2
     *  -------------  3
     * -3-2-1 0 1 2 3
     *----------------------------
     * Expected output structure:
     *----------------------------
     * should not trim the workspace at all.
     *  ------------- -3
     *  |/a/|   |/b/| -2
     *  ------------- -1
     *  |   |   |   |  0
     *  -------------  1
     *  |/c/|   |/d/|  2
     *  -------------  3
     * -3-2-1 0 1 2 3
     */
    using namespace Mantid::DataObjects;
    const size_t numDims = 2;
    const double signal = 0.0;
    const double errorSquared = 1.2;
    size_t numBins[static_cast<int>(numDims)] = {3, 3};
    Mantid::coord_t min[static_cast<int>(numDims)] = {-3, -3};
    Mantid::coord_t max[static_cast<int>(numDims)] = {3, 3};
    const std::string name("test");
    auto inWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    inWS->setSignalAt(0, 1.0); // cell a
    inWS->setSignalAt(2, 1.0); // cell b
    inWS->setSignalAt(6, 1.0); // cell c
    inWS->setSignalAt(8, 1.0); // cell d

    CompactMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    IMDHistoWorkspace_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS(
        "Should have a signal of 1.0: ", outputWorkspace->getSignalAt(0), 1);
    TSM_ASSERT_EQUALS(
        "Should have a signal of 1.0: ", outputWorkspace->getSignalAt(2), 1);
    TSM_ASSERT_EQUALS(
        "Should have a signal of 1.0: ", outputWorkspace->getSignalAt(6), 1);
    TSM_ASSERT_EQUALS(
        "Should have a signal of 1.0: ", outputWorkspace->getSignalAt(8), 1);
    TSM_ASSERT_EQUALS("Minimum for dim 0 should be consistent: ",
                      outputWorkspace->getDimension(0)->getMinimum(),
                      inWS->getDimension(0)->getMinimum());
    TSM_ASSERT_EQUALS("Maximum for dim 0 should be consistent: ",
                      outputWorkspace->getDimension(0)->getMaximum(),
                      inWS->getDimension(0)->getMaximum());
    TSM_ASSERT_EQUALS("Minimum for dim 1 should be consistent:",
                      outputWorkspace->getDimension(1)->getMinimum(),
                      inWS->getDimension(1)->getMinimum());
    TSM_ASSERT_EQUALS("Maximum for dim 1 should be consistent: ",
                      outputWorkspace->getDimension(1)->getMaximum(),
                      inWS->getDimension(1)->getMaximum());
    TSM_ASSERT_EQUALS("Number of Bins for dim 0 should be consistent : ",
                      outputWorkspace->getDimension(0)->getNBins(),
                      inWS->getDimension(0)->getNBins());
    TSM_ASSERT_EQUALS("Number of Bins for dim 1 should be consistent : ",
                      outputWorkspace->getDimension(1)->getNBins(),
                      inWS->getDimension(1)->getNBins());
    TSM_ASSERT_EQUALS("Bin width for dim 0 should be consistent: ",
                      outputWorkspace->getDimension(0)->getBinWidth(),
                      inWS->getDimension(0)->getBinWidth());
    TSM_ASSERT_EQUALS("Bin width for dim 1 should be consistent: ",
                      outputWorkspace->getDimension(1)->getBinWidth(),
                      inWS->getDimension(1)->getBinWidth());
  }

  void
  test_all_non_zero_signals_are_kept_when_data_is_concentrated_in_one_half_of_the_workspace() {
    /*
     *testing the effectiveness of CompactMD when the data looks like this:
     *------------------
     * Input structure:
     *------------------
     *  -------------
     *  |///|   |   |
     *  -------------
     * -3-2-1 0 1 2 3
     *---------------------------
     * Expected output structure:
     *----------------------------
     * should trim until the first non-zero value.
     *  -----
     *  |///|
     *  -----
     *  1 2 3
     */

    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 0.0;
    const double errorSquared = 1.3;
    size_t numBins[static_cast<int>(numDims)] = {3};
    Mantid::coord_t min[static_cast<int>(numDims)] = {-3};
    Mantid::coord_t max[static_cast<int>(numDims)] = {3};
    const std::string name("test");
    auto inWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    inWS->setSignalAt(0, 1.0); // set right-most bin signal to one
    CompactMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    IMDHistoWorkspace_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWorkspace);
    TSM_ASSERT_EQUALS(
        "Should have a signal of 1.0: ", outputWorkspace->getSignalAt(0), 1);
    TSM_ASSERT_EQUALS("Minimum should be cut to 1: ",
                      outputWorkspace->getDimension(0)->getMinimum(), -3.0);
    TSM_ASSERT_EQUALS("Maximum should still be 3: ",
                      outputWorkspace->getDimension(0)->getMaximum(), -1.0);
    TSM_ASSERT_EQUALS("Number of Bins should be 1 : ",
                      outputWorkspace->getDimension(0)->getNBins(), 1);
    TSM_ASSERT_EQUALS("Bin width should be consistent: ",
                      outputWorkspace->getDimension(0)->getBinWidth(),
                      inWS->getDimension(0)->getBinWidth());
  }

  void test_compact_md_does_not_throw_when_loading_empty_workspace() {
    using namespace Mantid::DataObjects;
    const size_t numDims = 1;
    const double signal = 0.0;
    const double errorSquared = 1.3;
    size_t numBins[static_cast<int>(numDims)] = {3};
    Mantid::coord_t min[static_cast<int>(numDims)] = {-3};
    Mantid::coord_t max[static_cast<int>(numDims)] = {3};
    const std::string name("test");
    auto inWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    CompactMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }
};

//===================
// Performance Tests
//===================
using namespace Mantid::DataObjects;
class CompactMDTestPerformance : public CxxTest::TestSuite {

private:
  MDHistoWorkspace_sptr m_ws;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompactMDTestPerformance *createSuite() {
    return new CompactMDTestPerformance();
  }
  static void destroySuite(CompactMDTestPerformance *suite) { delete suite; }
  void setUp() override {
    // Create a 4D workspace.
    const size_t numDims = 4;
    const double signal = 0.0;
    const double errorSquared = 1.2;
    size_t numBins[static_cast<int>(numDims)] = {10, 20, 10, 20};
    Mantid::coord_t min[static_cast<int>(numDims)] = {-5, -10, -5, -10};
    Mantid::coord_t max[static_cast<int>(numDims)] = {5, 10, 5, 10};
    const std::string name("test");
    m_ws = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        numDims, signal, errorSquared, numBins, min, max, name);
    // setting signals like this for variety
    auto iter = m_ws->createIterator();
    do {
      auto index = iter->getLinearIndex();
      if (index % 2 == 0) {
        m_ws->setSignalAt(index, 1.0);
      }
    } while (iter->next());
  }
  void test_execute_4d() {
    CompactMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_ws);
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    IMDHistoWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
  }
};

#endif // !MANTID_MDALGORITHMS_COMPACTMDTEST_H_
