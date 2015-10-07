#ifndef MANTID_MDALGORITHMS_COMPACTMDTEST_H_
#define MANTID_MDALGORITHMS_COMPACTMDTEST_H_
#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using Mantid::MDAlgorithms::CompactMD;
using namespace Mantid::API;

namespace {

// This helper sets the signal values to the linear index to have some
// variety
void resetSignalsToLinearIndexValue(IMDHistoWorkspace_sptr ws) {
  auto numberOfIndices = static_cast<size_t>(ws->getNPoints());
  for (size_t i = 0; i < numberOfIndices; ++i) {
    auto &signal = ws->signalAt(i);
    signal = static_cast<Mantid::signal_t>(i);
  }
}
}

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
  test_that_all_non_zero_signals_are_kept_with_data_concentrated_in_the_centre() {
    /*
     *testing the effectiveness of CompactMD when the data looks like this:
     *------------------
     * Input structure:
     *------------------
     *  -------------
     *  |   |   |   |
     *  -------------
     *  |   |///|   |
     *  -------------
     *  |   |   |   |
     *  -------------
     * -3-2-1 0 1 2 3
     *---------------------------
     * Expected output structure:
     *----------------------------
     * should trim until the first non-zero value.
     *    -------
     *    |/////|
     *    -------
     *   -1  0  1
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
    inWS->setSignalAt(1, 1.0); // set middle bin signal to one
    CompactMD alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    // output workspace should be cropped so extents ~ [-1,1]
    IMDHistoWorkspace_sptr outputWorkspace = alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("Should have a signal of 1.0: ",
                      outputWorkspace->getSignalAt(0), 1);
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
  void test__all_non_zero_signals_are_kept_with_data_in_each_corner() {
    /*
     *testing the effectiveness of CompactMD when the data looks like this:
     *------------------
     * Input structure:
     *------------------
     *  -------------
     *  |///|   |///|
     *  -------------
     *  |   |   |   |
     *  -------------
     *  |///|   |///|
     *  -------------
     * -3-2-1 0 1 2 3
     *---------------------------
     * Expected output structure:
     *----------------------------
     * should not trim the workspace at all.
     *  -------------
     *  |///|   |///|
     *  -------------
     *  |   |   |   |
     *  -------------
     *  |///|   |///|
     *  -------------
     * -3-2-1 0 1 2 3
     */
  }
};
//===================
// Performance Tests
//===================
// TODO:
#endif // !MANTID_MDALGORITHMS_COMPACTMDTEST_H_
