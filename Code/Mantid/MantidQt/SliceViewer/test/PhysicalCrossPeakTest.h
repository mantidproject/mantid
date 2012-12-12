#ifndef SLICE_VIEWER_PHYSICAL_CROSS_PEAK_TEST_H_
#define SLICE_VIEWER_PHYSICAL_CROSS_PEAK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PhysicalCrossPeak.h"
#include "MockObjects.h"
#include <vector>

using namespace MantidQt::SliceViewer;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional Tests
//=====================================================================================
class PhysicalCrossPeakTest : public CxxTest::TestSuite
{
public:

  void test_constructor_defaults()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    // Scale 1:1 on both x and y for simplicity.
    const double windowHeight = 200;
    const double windowWidth = 200;

    auto drawObject = physicalPeak.draw(windowHeight, windowWidth);

    // Quick white-box calculations of the outputs to expect.
    TS_ASSERT_EQUALS(3, drawObject.peakHalfCrossWidth);
    TS_ASSERT_EQUALS(3, drawObject.peakHalfCrossHeight);
    TS_ASSERT_EQUALS(0, drawObject.peakOpacityAtDistance);
    TS_ASSERT_EQUALS(2, drawObject.peakLineWidth);
  }

  void test_setSlicePoint_to_intersect()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double slicePoint = 0.5;// set to be half way through the effective radius.
    physicalPeak.setSlicePoint(slicePoint);

    const double windowHeight = 200;
    const double windowWidth = 200;

    auto drawObject = physicalPeak.draw(windowHeight, windowWidth);

    // Quick white-box calculations of the outputs to expect.
    const double expectedLineWidth = 2;
    const int expectedHalfCrossWidth = int(windowWidth * 0.015);
    const int expectedHalfCrossHeight = int(windowHeight * 0.015);

    TS_ASSERT_EQUALS( expectedHalfCrossWidth, drawObject.peakHalfCrossWidth );
    TS_ASSERT_EQUALS( expectedHalfCrossHeight, drawObject.peakHalfCrossHeight );
    TS_ASSERT_EQUALS( expectedLineWidth, drawObject.peakLineWidth );
  }

  void test_movePosition(PeakTransform_sptr peakTransform)
  {
    MockPeakTransform* pMockTransform = new MockPeakTransform;
    EXPECT_CALL(*pMockTransform, transform(_)).Times(1);
    PeakTransform_sptr transform(pMockTransform);

    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);
    physicalPeak.movePosition(transform); // Should invoke the mock method.

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockTransform));
  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
class PhysicalCrossPeakTestPerformance : public CxxTest::TestSuite
{
private:

  /// Collection to store a large number of physicalPeaks.
  std::vector<PhysicalCrossPeak> m_physicalPeaks;

public:

  /**
  Here we create a distribution of Peaks. Peaks are dispersed. This is to give a measurable peformance.
  */
  PhysicalCrossPeakTestPerformance()
  {
    const int sizeInAxis = 110;
    const double maxZ = 110;
    const double minZ = 0;
    m_physicalPeaks.reserve(sizeInAxis*sizeInAxis*sizeInAxis);
    for(int x = 0; x < sizeInAxis; ++x)
    {
      for(int y =0; y < sizeInAxis; ++y)
      {
        for(int z = 0; z < sizeInAxis; ++z)
        {
          V3D peakOrigin(x, y, z);
          m_physicalPeaks.push_back(PhysicalCrossPeak(peakOrigin, maxZ, minZ));
        }
      }
    }
  }

  /// Test the performance of just setting the slice point.
  void test_setSlicePoint_performance()
  {
    std::vector<PhysicalCrossPeak>::iterator it = m_physicalPeaks.begin();
    const double z = 10;
    while(it != m_physicalPeaks.end())
    {
      (*it).setSlicePoint(z);
      ++it;
    }
  }

  /// Test the performance of just drawing.
  void test_draw_performance()
  {
    auto it = m_physicalPeaks.begin();
    while(it != m_physicalPeaks.end())
    {
      (*it).draw(1, 1);
      ++it;
    }
  }

  /// Test the performance of both setting the slice point and drawing..
  void test_whole_performance()
  {
    auto it = m_physicalPeaks.begin();
    const double z = 10;
    while(it != m_physicalPeaks.end())
    {
      (*it).setSlicePoint(z);
      (*it).draw(1, 1);
      ++it;
    }
  }

};

#endif

//end SLICE_VIEWER_PHYSICAL_CROSS_PEAK_TEST_H_