#ifndef SLICE_VIEWER_PHYSICAL_SPHERICAL_PEAK_TEST_H_
#define SLICE_VIEWER_PHYSICAL_SPHERICAL_PEAK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PhysicalSphericalPeak.h"
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
class PhysicalSphericalPeakTest : public CxxTest::TestSuite
{
public:

  void test_not_isViewable_after_construction()
  {
    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);

    TSM_ASSERT("Should NOT be viewable until a slice point < r is set.", !physicalPeak.isViewable());
  }

  void test_isViewable_after_setSlicePoint_to_intersect()
  {
    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);

    const double delta = 0.01;
    const double slicePoint = radius - delta;
    physicalPeak.setSlicePoint(slicePoint);

    TSM_ASSERT("Should be viewable since slice point < r.", physicalPeak.isViewable());
  }

  void test_not_isViewable_after_setSlicePoint_beyond_range()
  {
    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);

    const double delta = 0.01;
    const double slicePoint = radius + delta;
    physicalPeak.setSlicePoint(slicePoint);

    TSM_ASSERT("Should NOT be viewable until a slice point > r is set.", !physicalPeak.isViewable());
  }

  void test_draw_defaults()
  {
    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);

    // Scale 1:1 on both x and y for simplicity.
    const double windowHeight = 1;
    const double windowWidth = 1;
    const double viewHeight = 1;
    const double viewWidth = 1;

    TSM_ASSERT("Should NOT be viewable until a slice point < r is set.", !physicalPeak.isViewable());
    auto drawObject = physicalPeak.draw(windowHeight, windowWidth, viewHeight, viewWidth);

    // The Return object should be initialized to zero in every field.
    TS_ASSERT_EQUALS(0, drawObject.peakOpacityAtDistance);
    TS_ASSERT_EQUALS(0, drawObject.peakOuterRadiusX);
    TS_ASSERT_EQUALS(0, drawObject.peakOuterRadiusY);
    TS_ASSERT_EQUALS(0, drawObject.peakLineWidth);
  }

  void test_setSlicePoint_to_intersect_and_draw()
  {
    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);

    const double slicePoint = 0.5;// set to be half way through the radius.
    physicalPeak.setSlicePoint(slicePoint);

    // Scale 1:1 on both x and y for simplicity.
    const double windowHeight = 1;
    const double windowWidth = 1;
    const double viewHeight = 1;
    const double viewWidth = 1;

    TSM_ASSERT("Should be viewable since slice point < r is set.", physicalPeak.isViewable());
    auto drawObject = physicalPeak.draw(windowHeight, windowWidth, viewHeight, viewWidth);

    // Quick white-box calculations of the outputs to expect.
    const double expectedOpacityAtDistance = (0.8 - 0)/2;
    const double expectedLineWidth = radius - std::sqrt( std::pow(radius, 2) - std::pow(radius/2, 2));
    const double expectedRadius = radius - (expectedLineWidth/2);

    TS_ASSERT_EQUALS( expectedOpacityAtDistance, drawObject.peakOpacityAtDistance);
    TS_ASSERT_EQUALS( expectedRadius, drawObject.peakOuterRadiusX);
    TS_ASSERT_EQUALS( expectedRadius, drawObject.peakOuterRadiusY);
    TS_ASSERT_EQUALS( expectedLineWidth, drawObject.peakLineWidth);
  }

  void test_movePosition()
  {
    MockPeakTransform* pMockTransform = new MockPeakTransform;
    EXPECT_CALL(*pMockTransform, transform(_)).Times(1).WillOnce(Return(V3D(0,0,0)));
    PeakTransform_sptr transform(pMockTransform);

    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);
    physicalPeak.movePosition(transform); // Should invoke the mock method.

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockTransform));
  }

};

//=====================================================================================
// Performance Tests
//=====================================================================================
class PhysicalSphericalPeakTestPerformance : public CxxTest::TestSuite
{
private:

  typedef boost::shared_ptr<PhysicalSphericalPeak> PhysicalSpericalPeak_sptr;
  typedef std::vector<PhysicalSpericalPeak_sptr > VecPhysicalSphericalPeak;

  /// Collection to store a large number of physicalPeaks.
  VecPhysicalSphericalPeak m_physicalPeaks;

public:

  /**
  Here we create a distribution of Peaks. Peaks are dispersed. This is to give a measurable peformance.
  */
  PhysicalSphericalPeakTestPerformance()
  {
    const int sizeInAxis = 50;
    const double radius = 5;
    m_physicalPeaks.reserve(sizeInAxis*sizeInAxis*sizeInAxis);
    for(int x = 0; x < sizeInAxis; ++x)
    {
      for(int y =0; y < sizeInAxis; ++y)
      {
        for(int z = 0; z < sizeInAxis; ++z)
        {
          V3D peakOrigin(x, y, z);
          m_physicalPeaks.push_back(boost::make_shared<PhysicalSphericalPeak>(peakOrigin, radius));
        }
      }
    }
  }

  /// Test the performance of just setting the slice point.
  void test_setSlicePoint_performance()
  {
    for(double z = 0; z < 50; z+=5)
    {
      VecPhysicalSphericalPeak::iterator it = m_physicalPeaks.begin();
      while(it != m_physicalPeaks.end())
      {
        PhysicalSpericalPeak_sptr physicalPeak = *it;
        physicalPeak->setSlicePoint(z);
        ++it;
      }
    }
  }

  /// Test the performance of just drawing.
  void test_draw_performance()
  {
    const int nTimesRedrawAll = 10;
    int timesDrawn = 0;
    while(timesDrawn < nTimesRedrawAll)
    {
      // Set the slicing point on all peaks.
      VecPhysicalSphericalPeak::iterator it = m_physicalPeaks.begin();
      while(it != m_physicalPeaks.end())
      {
        (*it)->draw(1, 1, 1, 1);
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
      (*it)->draw(1, 1, 1, 1);
      ++it;
    }
  }

};

#endif

//end SLICE_VIEWER_PEAKTRANSFORMHKL_TEST_H_