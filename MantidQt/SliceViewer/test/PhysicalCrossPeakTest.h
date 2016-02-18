#ifndef SLICE_VIEWER_PHYSICAL_CROSS_PEAK_TEST_H_
#define SLICE_VIEWER_PHYSICAL_CROSS_PEAK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PhysicalCrossPeak.h"
#include "MockObjects.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace MantidQt::SliceViewer;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional Tests
//=====================================================================================
class PhysicalCrossPeakTest : public CxxTest::TestSuite
{
public:

  void test_setSlicePoint_to_intersect()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double slicePoint = 0;
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

  void test_movePosition()
  {
    MockPeakTransform* pMockTransform = new MockPeakTransform;
    EXPECT_CALL(*pMockTransform, transform(_)).Times(1).WillOnce(Return(V3D(0,0,0)));
    PeakTransform_sptr transform(pMockTransform);

    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);
    physicalPeak.movePosition(transform); // Should invoke the mock method.

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockTransform));
  }

    void test_getBoundingBox()
  {
    /*

    width = height = effectiveradius * 2
    |---------------|
    |               |
    |               |
    |     (0,0)     |
    |               |
    |               |
    |---------------|

    */
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);
    
    // Pre-calculate the effective radius.
    const double effectiveRadius = 0.015 * (maxZ - minZ);

    auto boundingBox = physicalPeak.getBoundingBox();

    const double expectedLeft(origin.X() - effectiveRadius);
    const double expectedBottom(origin.Y() - effectiveRadius);
    const double expectedRight(origin.X() + effectiveRadius);
    const double expectedTop(origin.Y() + effectiveRadius);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

  void test_getBoundingBox_with_offset_origin()
  {
    /*

    width = height = effectiveradius * 2
    |---------------|
    |               |
    |               |
    |     (-1,1)    |
    |               |
    |               |
    |---------------|

    */

    V3D origin(-1, 1, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);
    
    // Pre-calculate the effective radius.
    const double effectiveRadius = 0.015 * (maxZ - minZ);

    auto boundingBox = physicalPeak.getBoundingBox();

    const double expectedLeft(origin.X() - effectiveRadius);
    const double expectedBottom(origin.Y() - effectiveRadius);
    const double expectedRight(origin.X() + effectiveRadius);
    const double expectedTop(origin.Y() + effectiveRadius);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

  void test_expand_peak_intoplane()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double newEffectiveRadiusFactor = 0.2;
    const double effectiveRadius = newEffectiveRadiusFactor * (maxZ - minZ);
    physicalPeak.setOccupancyIntoView(newEffectiveRadiusFactor);
    TS_ASSERT_EQUALS(newEffectiveRadiusFactor, physicalPeak.getOccupancyIntoView());
    TS_ASSERT_EQUALS(effectiveRadius, physicalPeak.getEffectiveRadius());
  }

  void test_expand_peak_inplane()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double occupancyFraction = 0.01; // 1%
    physicalPeak.setOccupancyInView(occupancyFraction);// 1 %
    auto drawingObject = physicalPeak.draw(1000, 1000);
    TS_ASSERT_EQUALS(occupancyFraction, physicalPeak.getOccupancyInView());
  }

  void test_setOccupanyIntoView_ignores_zeros()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    double defaultOccupancy = physicalPeak.getOccupancyIntoView();

    // Now try to set it to zero.
    physicalPeak.setOccupancyIntoView(0);

    TSM_ASSERT_DIFFERS("Should have ignored the zero value input", 0,
        physicalPeak.getOccupancyIntoView());
    TS_ASSERT_EQUALS(defaultOccupancy, physicalPeak.getOccupancyIntoView());

  }


};

//=====================================================================================
// Performance Tests
//=====================================================================================
class PhysicalCrossPeakTestPerformance : public CxxTest::TestSuite
{
private:

  typedef std::vector<boost::shared_ptr<PhysicalCrossPeak> > VecPhysicalCrossPeak;

  /// Collection to store a large number of physicalPeaks.
  VecPhysicalCrossPeak m_physicalPeaks;

public:

  /**
  Here we create a distribution of Peaks. Peaks are dispersed. This is to give a measurable peformance.
  */
  PhysicalCrossPeakTestPerformance()
  {
    const int sizeInAxis = 100;
    const double maxZ = 100;
    const double minZ = 0;
    m_physicalPeaks.reserve(sizeInAxis*sizeInAxis*sizeInAxis);
    for(int x = 0; x < sizeInAxis; ++x)
    {
      for(int y =0; y < sizeInAxis; ++y)
      {
        for(int z = 0; z < sizeInAxis; ++z)
        {
          V3D peakOrigin(x, y, z);
          m_physicalPeaks.push_back(boost::make_shared<PhysicalCrossPeak>(peakOrigin, maxZ, minZ));
        }
      }
    }
  }

  /// Test the performance of just setting the slice point.
  void test_setSlicePoint_performance()
  {
    for(double z = 0; z < 100; z+=5)
    {
      VecPhysicalCrossPeak::iterator it = m_physicalPeaks.begin();
      while(it != m_physicalPeaks.end())
      {
        (*it)->setSlicePoint(z);
        ++it;
      }
    }
  }

  /// Test the performance of just drawing.
  void test_draw_performance()
  {
    const int nTimesRedrawAll = 20;
    int timesDrawn = 0;
    while(timesDrawn < nTimesRedrawAll)
    {
      auto it = m_physicalPeaks.begin();
      while(it != m_physicalPeaks.end())
      {
        (*it)->draw(1, 1);
        ++it;
      }
      ++timesDrawn;
    }
  }

  /// Test the performance of both setting the slice point and drawing..
  void test_whole_performance()
  {
    auto it = m_physicalPeaks.begin();
    const double z = 10;
    while(it != m_physicalPeaks.end())
    {
      (*it)->setSlicePoint(z);
      (*it)->draw(1, 1);
      ++it;
    }
  }

};

#endif

//end SLICE_VIEWER_PHYSICAL_CROSS_PEAK_TEST_H_
