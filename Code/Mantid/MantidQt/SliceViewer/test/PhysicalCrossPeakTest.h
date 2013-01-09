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

  void test_not_isViewable_after_construction()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    TSM_ASSERT("Should NOT be viewable until a slice point < r is set.", !physicalPeak.isViewable());
  }

  void test_isViewable_after_setSlicePoint_to_intersect()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double delta = 0.01;
    const double effectiveRadius = 0.015 * (maxZ - minZ);
    const double slicePoint = effectiveRadius - delta;
    physicalPeak.setSlicePoint(slicePoint);

    TSM_ASSERT("Should be viewable since slice point < r.", physicalPeak.isViewable());
  }

  void test_not_isViewable_after_setSlicePoint_beyond_range()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double delta = 0.01;
    const double effectiveRadius = 0.015 * (maxZ - minZ);
    const double slicePoint = effectiveRadius + delta;
    physicalPeak.setSlicePoint(slicePoint);

    TSM_ASSERT("Should NOT be viewable until a slice point > r is set.", !physicalPeak.isViewable());
  }

  void test_constructor_defaults()
  {
    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    // Scale 1:1 on both x and y for simplicity.
    const double windowHeight = 200;
    const double windowWidth = 200;

    TSM_ASSERT("Should NOT be viewable until a slice point > r is set.", !physicalPeak.isViewable());
    auto drawObject = physicalPeak.draw(windowHeight, windowWidth);

    // Quick white-box calculations of the outputs to expect.
    TS_ASSERT_EQUALS(0, drawObject.peakHalfCrossWidth);
    TS_ASSERT_EQUALS(0, drawObject.peakHalfCrossHeight);
    TS_ASSERT_EQUALS(0, drawObject.peakOpacityAtDistance);
    TS_ASSERT_EQUALS(0, drawObject.peakLineWidth);
  }

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
    auto lowerLeft = boundingBox.get<0>();
    auto upperRight = boundingBox.get<1>();

    TS_ASSERT_EQUALS(V2D(-effectiveRadius, -effectiveRadius), lowerLeft);
    TS_ASSERT_EQUALS(V2D(effectiveRadius, effectiveRadius), upperRight);
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

    V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);
    
    auto boundingBox = physicalPeak.getBoundingBox();
    auto lowerLeft = boundingBox.get<0>();
    auto upperRight = boundingBox.get<1>();

    // Pre-calculate the effective radius.
    const double effectiveRadius = 0.015 * (maxZ - minZ);

    const V2D expectedLowerLeft(origin.X() - effectiveRadius, origin.Y() - effectiveRadius);
    const V2D expectedUpperRight(origin.X() + effectiveRadius, origin.Y() + effectiveRadius);

    TS_ASSERT_EQUALS(expectedLowerLeft, lowerLeft);
    TS_ASSERT_EQUALS(expectedUpperRight, upperRight);
  }

  void test_getBoundingBox_windows_coordinates()
  {
    /*
    calculated in natural coords: width = height = outerradius * 2
    in windows coords: window height = 100, window width = 50
    |---------------|
    |               |
    |               |
    |     (0,0)     |
    |               |
    |               |
    |---------------|

    */

    V3D origin(0, 0, 0); // Offset origin from (0, 0, 0)
    const double maxZ = 1;
    const double minZ = 0;
    PhysicalCrossPeak physicalPeak(origin, maxZ, minZ);

    const double viewHeight = 1;
    const double viewWidth = 1;
    const double windowHeight = 100;
    const double windowWidth = 50;
    
    auto boundingBox = physicalPeak.getBoundingBox(windowHeight, windowWidth, viewWidth, viewHeight);
    auto lowerLeft = boundingBox.get<0>();
    auto upperRight = boundingBox.get<1>();

    double scaleXFactor = windowWidth/viewWidth; // To convert the box into windows coordinates
    double scaleYFactor = windowHeight/viewHeight; // To convert the box into windows coordinates

    // Pre-calculate the effective radius.
    const double effectiveRadius = 0.015 * (maxZ - minZ);

    const double expectedLowerLeftX = (origin.X() - effectiveRadius) * scaleXFactor;
    const double expectedLowerLeftY = (origin.Y() - effectiveRadius) * scaleYFactor;
    const double expectedUpperRightX = (origin.X() + effectiveRadius) * scaleXFactor;
    const double expectedUpperRightY = (origin.Y() + effectiveRadius) * scaleYFactor;

    TS_ASSERT_EQUALS(expectedLowerLeftX, lowerLeft.X());
    TS_ASSERT_EQUALS(expectedLowerLeftY, lowerLeft.Y());
    TS_ASSERT_EQUALS(expectedUpperRightX, upperRight.X());
    TS_ASSERT_EQUALS(expectedUpperRightY, upperRight.Y());
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