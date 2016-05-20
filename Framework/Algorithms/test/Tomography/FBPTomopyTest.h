#ifndef MANTID_ALGORITHMS_FBPTOMOPYTEST_H_
#define MANTID_ALGORITHMS_FBPTOMOPYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Tomography/FBPTomopy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Exception.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <numeric>

using namespace Mantid::Algorithms::Tomography;

class FBPTomopyTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FBPTomopyTest *createSuite() { return new FBPTomopyTest(); }

  static void destroySuite(FBPTomopyTest *suite) { delete suite; }

  void test_null() {
    FBPTomopy(nullptr, 0, 0, 0, nullptr, nullptr, nullptr, 0, 0);
  }

  void test_small_buffer_flat() {
    const size_t numProj = 3;
    const size_t ysize = 8;
    const size_t xsize = 8;
    const size_t projSize = numProj * ysize * xsize;
    std::array<float, projSize> projImages;
    std::fill(projImages.begin(), projImages.end(), 33.0f);

    const size_t reconSize = ysize * ysize * xsize;
    std::array<float, reconSize> reconVol;
    std::fill(reconVol.begin(), reconVol.end(), 0.0f);

    const size_t numAngles = numProj;
    std::array<float, numAngles> angles{{0.0, 90.0, 180.0}};

    const size_t numCenters = ysize;
    std::array<float, numCenters> centers{{4, 3, 3, 4, 4, 4, 4, 3}};

    FBPTomopy(projImages.data(), ysize, numProj, xsize, centers.data(),
              angles.data(), reconVol.data(), xsize, ysize);

    TS_ASSERT_DELTA(reconVol.front(), 66.7519, 1);
    TS_ASSERT_DELTA(reconVol[1], 67.9431, 1);
    TS_ASSERT_DELTA(reconVol[2], 90.9802, 1);
    TS_ASSERT_DELTA(reconVol[3], 111.1035, 1);
    TS_ASSERT_DELTA(reconVol[4], 88.6847, 1);
    TS_ASSERT_DELTA(reconVol[20], 102.3709, 1);
    TS_ASSERT_DELTA(reconVol[50], 97.4173, 1);
    TS_ASSERT_DELTA(reconVol[100], 97.4172, 1);
    TS_ASSERT_DELTA(reconVol[150], 99.8273, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 5], 82.4248, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 4], 74.1906, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 3], 60.5043, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 2], 67.9431, 1);
    TS_ASSERT_DELTA(reconVol.back(), 66.7519, 1);
  }

  void test_buffer_idx() {
    const size_t numProj = 8;
    const size_t ysize = 16;
    const size_t xsize = 16;
    const size_t projSize = numProj * ysize * xsize;
    std::array<float, projSize> projImages;
    std::iota(projImages.begin(), projImages.end(), 0.0f);

    // inconsistent/stressing values
    std::fill(projImages.begin() + 300, projImages.begin() + 400, 333.0f);
    std::fill(projImages.begin() + 600, projImages.begin() + 850, 999.0f);
    std::fill(projImages.begin() + 990, projImages.begin() + 1100, 1000.0f);
    std::fill(projImages.begin() + 1500, projImages.begin() + 1700, -444.0f);
    std::fill(projImages.begin() + 1900, projImages.begin() + 2000, 765.0f);

    const size_t reconSize = ysize * ysize * xsize;
    std::array<float, reconSize> reconVol;
    std::fill(reconVol.begin(), reconVol.end(), 0.0f);

    const size_t numAngles = numProj;
    std::array<float, numAngles> angles{
        {0.0, 45.0, 90.0, 135.0, 180.0, 225.0, 270.0, 315.0}};

    const size_t numCenters = ysize;
    std::array<float, numCenters> centers;
    std::fill(centers.begin(), centers.end(), 7.5f);

    FBPTomopy(projImages.data(), ysize, numProj, xsize, centers.data(),
              angles.data(), reconVol.data(), xsize, ysize);

    TS_ASSERT_DELTA(reconVol.front(), 241.6610, 1);
    TS_ASSERT_DELTA(reconVol[1], 286.6727, 1);
    TS_ASSERT_DELTA(reconVol[2], 392.6853, 1);
    TS_ASSERT_DELTA(reconVol[3], 350.0282, 1);
    TS_ASSERT_DELTA(reconVol[4], 429.8395, 1);
    TS_ASSERT_DELTA(reconVol[200], 516.5272, 1);
    TS_ASSERT_DELTA(reconVol[500], 1202.6435, 1);
    TS_ASSERT_DELTA(reconVol[1000], 3604.5090, 1);
    TS_ASSERT_DELTA(reconVol[1500], 8092.9765, 1);
    TS_ASSERT_DELTA(reconVol[1900], 7941.9135, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 5], 6829.1318, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 4], 5662.6342, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 3], 5747.1845, 1);
    TS_ASSERT_DELTA(reconVol[projSize - 2], 6034.2163, 1);
    TS_ASSERT_DELTA(reconVol.back(), 3736.5483, 1);
  }
};

#endif /* MANTID_ALGORITHM_FBPTOMOPYTEST_H_ */
