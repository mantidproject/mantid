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

  void test_constructor_defaults()
  {
    V3D origin(0, 0, 0);
    const double radius = 1;
    PhysicalSphericalPeak physicalPeak(origin, radius);

    // Scale 1:1 on both x and y for simplicity.
    const double windowHeight = 1;
    const double windowWidth = 1;
    const double viewHeight = 1;
    const double viewWidth = 1;

    auto drawObject = physicalPeak.draw(windowHeight, windowWidth, viewHeight, viewWidth);

    // Quick white-box calculations of the outputs to expect.
    TS_ASSERT_EQUALS(0, drawObject.peakOpacityAtDistance);
    TS_ASSERT_EQUALS(0.5, drawObject.peakOuterRadiusX);
    TS_ASSERT_EQUALS(0.5, drawObject.peakOuterRadiusY);
    TS_ASSERT_EQUALS(1, drawObject.peakLineWidth);
  }

  void test_setSlicePoint_to_intersect()
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

  typedef std::vector<boost::shared_ptr<PhysicalSphericalPeak> > VecPhysicalSphericalPeak;

  /// Collection to store a large number of physicalPeaks.
  VecPhysicalSphericalPeak m_physicalPeaks;

public:

  /**
  Here we create a distribution of Peaks. Peaks are dispersed. This is to give a measurable peformance.
  */
  PhysicalSphericalPeakTestPerformance()
  {
    const int sizeInAxis = 110;
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
    VecPhysicalSphericalPeak::iterator it = m_physicalPeaks.begin();
    const double z = 10;
    while(it != m_physicalPeaks.end())
    {
      (*it)->setSlicePoint(z);
      ++it;
    }
  }

  /// Test the performance of just drawing.
  void test_draw_performance()
  {
    auto it = m_physicalPeaks.begin();
    while(it != m_physicalPeaks.end())
    {
      (*it)->draw(1, 1, 1, 1);
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
      (*it)->setSlicePoint(z);
      (*it)->draw(1, 1, 1, 1);
      ++it;
    }
  }

};

#endif

//end SLICE_VIEWER_PEAKTRANSFORMHKL_TEST_H_