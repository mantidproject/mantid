#ifndef SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_
#define SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_

#include "MantidQtWidgets/SliceViewer/PeakRepresentationCross.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::SliceViewer;
using namespace testing;

class PeakRepresentationCrossExposeProtectedWrapper
    : public PeakRepresentationCross {
public:
  PeakRepresentationCrossExposeProtectedWrapper(
      const Mantid::Kernel::V3D &origin, const double &maxZ, const double &minZ)
      : PeakRepresentationCross(origin, maxZ, minZ) {}

  double getOccupancyInView() { return m_crossViewFraction; }

  double getOccupancyIntoView() { return m_intoViewFraction; }

  std::shared_ptr<PeakPrimitives> getDrawingInformationFromWrapper(
      PeakRepresentationViewInformation viewInformation) {
    return this->getDrawingInformation(viewInformation);
  }
};

class PeakRepresentationCrossTest : public CxxTest::TestSuite {
public:
  void test_movePosition_moves_the_peak() {

    // Arrange
    Mantid::Kernel::V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PeakRepresentationCross peak(origin, maxZ, minZ);

    /* Provide a mocked transform */
    MockPeakTransform *mockTransform = new MockPeakTransform;
    EXPECT_CALL(*mockTransform, transform(_))
        .Times(1)
        .WillOnce(Return(Mantid::Kernel::V3D(0, 0, 0)));
    PeakTransform_sptr transform(mockTransform);

    // Act
    peak.movePosition(transform);

    // Assert
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockTransform));
  }

  void test_getBoundingBox_gets_correct_box_without_offset_from_origin() {
    // Arrange

    /*
    width = height = effectiveradius * 2
    |---------------|
    |               |
    |               |
    |     (0,0)     |
    |               |
    |               |
    |----------------
    */
    Mantid::Kernel::V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PeakRepresentationCross peak(origin, maxZ, minZ);

    // Act
    auto boundingBox = peak.getBoundingBox();

    // Assert
    // Pre-calculate the effective radius.
    const double effectiveRadius = 0.015 * (maxZ - minZ);
    const double expectedLeft(origin.X() - effectiveRadius);
    const double expectedBottom(origin.Y() - effectiveRadius);
    const double expectedRight(origin.X() + effectiveRadius);
    const double expectedTop(origin.Y() + effectiveRadius);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

  void test_getBoundingBox_gets_correct_box_with_offset_from_origin() {
    // Arrange
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

    Mantid::Kernel::V3D origin(-1, 1, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PeakRepresentationCross peak(origin, maxZ, minZ);

    // Act
    auto boundingBox = peak.getBoundingBox();

    // Assert
    // Pre-calculate the effective radius.
    const double effectiveRadius = 0.015 * (maxZ - minZ);
    const double expectedLeft(origin.X() - effectiveRadius);
    const double expectedBottom(origin.Y() - effectiveRadius);
    const double expectedRight(origin.X() + effectiveRadius);
    const double expectedTop(origin.Y() + effectiveRadius);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

  void test_expand_peak_intoplane() {
    // Arrange
    Mantid::Kernel::V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PeakRepresentationCrossExposeProtectedWrapper peak(origin, maxZ, minZ);

    const double newEffectiveRadiusFactor = 0.2;
    const double effectiveRadius = newEffectiveRadiusFactor * (maxZ - minZ);

    // Act
    peak.setOccupancyIntoView(newEffectiveRadiusFactor);
    auto updatedOccupancyIntoView = peak.getOccupancyIntoView();
    auto updatedEffectiveRadius = peak.getEffectiveRadius();

    // Assert
    TS_ASSERT_EQUALS(newEffectiveRadiusFactor, updatedOccupancyIntoView);
    TS_ASSERT_EQUALS(effectiveRadius, updatedEffectiveRadius);
  }

  void test_expand_peak_inplane() {
    // Arrange
    Mantid::Kernel::V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PeakRepresentationCrossExposeProtectedWrapper peak(origin, maxZ, minZ);

    const double occupancyFraction = 0.01; // 1%

    // Act
    peak.setOccupancyInView(occupancyFraction); // 1 %
    auto updatedOccupancyInView = peak.getOccupancyInView();

    // Assert
    TS_ASSERT_EQUALS(occupancyFraction, updatedOccupancyInView);
  }

  void test_setOccupanyIntoView_ignores_zeros() {
    // Act
    Mantid::Kernel::V3D origin(0, 0, 0);
    const double maxZ = 1;
    const double minZ = 0;
    PeakRepresentationCrossExposeProtectedWrapper peak(origin, maxZ, minZ);

    // Act
    double defaultOccupancy = peak.getOccupancyIntoView();
    peak.setOccupancyIntoView(0);

    // Assert
    TSM_ASSERT_DIFFERS("Should have ignored the zero value input", 0,
                       peak.getOccupancyIntoView());
    TS_ASSERT_EQUALS(defaultOccupancy, peak.getOccupancyIntoView());
  }
};

//---------------------------------------------------------------------
// Performance Test
//---------------------------------------------------------------------

class PeakRepresentationCrossTestPerformance : public CxxTest::TestSuite {
private:
  using VecPeakRepCross =
      std::vector<boost::shared_ptr<PeakRepresentationCross>>;

  using VecPeakRepCrossWrapped = std::vector<
      boost::shared_ptr<PeakRepresentationCrossExposeProtectedWrapper>>;

  /// Collection to store a large number of PeakRepresentationCross.
  VecPeakRepCross m_peaks;

  /// Collection to store a large number of
  /// PeakRepresentationCrossProtectedWrapper.
  VecPeakRepCrossWrapped m_peaksWrapped;

public:
  /**
  Here we create a distribution of Peaks. Peaks are dispersed. This is to give
  a measurable peformance.
  */
  PeakRepresentationCrossTestPerformance() {
    const int sizeInAxis = 100;
    const double maxZ = 100;
    const double minZ = 0;
    m_peaks.reserve(sizeInAxis * sizeInAxis * sizeInAxis);
    for (int x = 0; x < sizeInAxis; ++x) {
      for (int y = 0; y < sizeInAxis; ++y) {
        for (int z = 0; z < sizeInAxis; ++z) {
          Mantid::Kernel::V3D peakOrigin(x, y, z);
          m_peaks.push_back(boost::make_shared<
                            MantidQt::SliceViewer::PeakRepresentationCross>(
              peakOrigin, maxZ, minZ));
        }
      }
    }
  }

  /// Test the performance of just setting the slice point.
  void test_setSlicePoint_performance() {
    for (double z = 0; z < 100; z += 5) {
      VecPeakRepCross::iterator it = m_peaks.begin();
      while (it != m_peaks.end()) {
        (*it)->setSlicePoint(z);
        ++it;
      }
    }
  }

  /// Test the performance of just drawing.
  void test_draw_performance() {
    const int nTimesRedrawAll = 20;
    int timesDrawn = 0;

    // View Settings Scale 1:1 on both x and y for simplicity.
    MantidQt::SliceViewer::PeakRepresentationViewInformation viewInformation;
    viewInformation.viewHeight = 1.0;
    viewInformation.viewWidth = 1.0;
    viewInformation.windowHeight = 1.0;
    viewInformation.windowWidth = 1.0;
    viewInformation.xOriginWindow = 1;
    viewInformation.yOriginWindow = 1;

    while (timesDrawn < nTimesRedrawAll) {
      auto it = m_peaksWrapped.begin();
      while (it != m_peaksWrapped.end()) {
        (*it)->getDrawingInformationFromWrapper(viewInformation);
        ++it;
      }
      ++timesDrawn;
    }
  }

  /// Test the performance of both setting the slice point and drawing..
  void test_whole_performance() {
    const double z = 10;

    // View Settings Scale 1:1 on both x and y for simplicity.
    MantidQt::SliceViewer::PeakRepresentationViewInformation viewInformation;
    viewInformation.viewHeight = 1.0;
    viewInformation.viewWidth = 1.0;
    viewInformation.windowHeight = 1.0;
    viewInformation.windowWidth = 1.0;
    viewInformation.xOriginWindow = 1;
    viewInformation.yOriginWindow = 1;

    auto it = m_peaksWrapped.begin();
    while (it != m_peaksWrapped.end()) {
      (*it)->setSlicePoint(z);
      (*it)->getDrawingInformationFromWrapper(viewInformation);
      ++it;
    }
  }
};

#endif
