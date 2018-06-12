#ifndef SLICE_VIEWER_PEAK_REPRESENTATION_SPHERE_TEST_H_
#define SLICE_VIEWER_PEAK_REPRESENTATION_SPHERE_TEST_H_

#include "MantidQtWidgets/SliceViewer/PeakRepresentationSphere.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::SliceViewer;
using namespace testing;

class PeakRepresentationSphereExposeProtectedWrapper
    : public PeakRepresentationSphere {
public:
  PeakRepresentationSphereExposeProtectedWrapper(
      const Mantid::Kernel::V3D &origin, const double &peakRadius,
      const double &backgroundInnerRadius, const double &backgroundOuterRadius)
      : PeakRepresentationSphere(origin, peakRadius, backgroundInnerRadius,
                                 backgroundOuterRadius) {}
  std::shared_ptr<PeakPrimitives> getDrawingInformationWrapper(
      PeakRepresentationViewInformation viewInformation) {
    return getDrawingInformation(viewInformation);
  }
};

class PeakRepresentationSphereTest : public CxxTest::TestSuite {
public:
  void test_getRadius_gets_radius_if_background_is_not_shown() {
    // Arrange
    Mantid::Kernel::V3D origin(0, 0, 0);
    const double radius = 1;
    const double innerBackgroundRadius = 2;
    const double outerBackgroundRadius = 3;
    PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                  outerBackgroundRadius);

    // Act + Assert
    TSM_ASSERT(radius, peak.getEffectiveRadius());
  }

  void test_getRadius_gets_outer_background_radius_if_background_is_shown() {
    // Arrange
    const Mantid::Kernel::V3D origin(0, 0, 0);
    const double radius = 1;
    const double innerBackgroundRadius = 2;
    const double outerBackgroundRadius = 3;
    PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                  outerBackgroundRadius);

    peak.showBackgroundRadius(true);

    // Act + Assert
    TSM_ASSERT(outerBackgroundRadius, peak.getEffectiveRadius());
  }

  void test_handle_outer_background_radius_zero() {
    // Arrange
    const Mantid::Kernel::V3D origin(0, 0, 0);
    const double radius = 1;
    const double innerBackgroundRadius = 2;
    const double outerBackgroundRadius =
        0; // This can happen using IntegratePeaksMD.
    PeakRepresentationSphereExposeProtectedWrapper peak(
        origin, radius, innerBackgroundRadius, outerBackgroundRadius);

    peak.showBackgroundRadius(true);

    const double slicePoint = innerBackgroundRadius;
    peak.setSlicePoint(slicePoint);

    // View Settings Scale 1:1 on both x and y for simplicity.
    PeakRepresentationViewInformation viewInformation;
    viewInformation.viewHeight = 1.0;
    viewInformation.viewWidth = 1.0;
    viewInformation.windowHeight = 1.0;
    viewInformation.windowWidth = 1.0;
    viewInformation.xOriginWindow = 1;
    viewInformation.yOriginWindow = 1;

    // Act
    auto drawingInformation =
        peak.getDrawingInformationWrapper(viewInformation);

    // Assert
    auto drawingInformationSphere =
        std::static_pointer_cast<PeakPrimitiveCircle>(drawingInformation);
    // The Return object should be initialized to zero in every field.
    TS_ASSERT_EQUALS(drawingInformationSphere->backgroundOuterRadiusX,
                     drawingInformationSphere->backgroundInnerRadiusX);
    TS_ASSERT_EQUALS(drawingInformationSphere->backgroundOuterRadiusY,
                     drawingInformationSphere->backgroundInnerRadiusY);
  }

  void
  test_that_setting_slice_point_to_intersect_produces_valid_drawing_information() {
    // Arrange
    const Mantid::Kernel::V3D origin(0, 0, 0);
    const double radius = 1;
    const double innerBackgroundRadius = 2;
    const double outerBackgroundRadius = 3;
    PeakRepresentationSphereExposeProtectedWrapper peak(
        origin, radius, innerBackgroundRadius, outerBackgroundRadius);

    const double slicePoint =
        radius / 2; // set to be half way through the radius.
    peak.setSlicePoint(slicePoint);

    // View Settings Scale 1:1 on both x and y for simplicity.
    PeakRepresentationViewInformation viewInformation;
    viewInformation.viewHeight = 1.0;
    viewInformation.viewWidth = 1.0;
    viewInformation.windowHeight = 1.0;
    viewInformation.windowWidth = 1.0;
    viewInformation.xOriginWindow = 1;
    viewInformation.yOriginWindow = 1;

    // Act
    auto drawingInformation =
        peak.getDrawingInformationWrapper(viewInformation);

    // Assert
    auto drawingInformationSphere =
        std::static_pointer_cast<PeakPrimitiveCircle>(drawingInformation);

    // Quick white-box calculations of the outputs to expect.
    const double expectedOpacityAtDistance = (0.8 - 0) / 2;
    auto peakRadSQ = std::pow(radius, 2);
    auto planeDistanceSQ = std::pow((slicePoint - origin.Z()), 2);
    const double expectedRadius = std::sqrt(peakRadSQ - planeDistanceSQ);

    TS_ASSERT_EQUALS(expectedOpacityAtDistance,
                     drawingInformationSphere->peakOpacityAtDistance);
    TS_ASSERT_EQUALS(expectedRadius,
                     drawingInformationSphere->peakInnerRadiusX);
    TS_ASSERT_EQUALS(expectedRadius,
                     drawingInformationSphere->peakInnerRadiusY);
  }

  void test_move_position_produces_correct_position() {
    // Arrange
    MockPeakTransform *pMockTransform = new MockPeakTransform;
    EXPECT_CALL(*pMockTransform, transform(_))
        .Times(1)
        .WillOnce(Return(Mantid::Kernel::V3D(0, 0, 0)));
    PeakTransform_sptr transform(pMockTransform);

    const Mantid::Kernel::V3D origin(0, 0, 0);
    const double radius = 1;
    const double innerBackgroundRadius = 2;
    const double outerBackgroundRadius = 3;
    PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                  outerBackgroundRadius);

    // Act
    peak.movePosition(transform);

    // Assert
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockTransform));
  }

  void test_getBoundingBox() {
    /*

    width = height = outerradius * 2
    |---------------|
    |               |
    |               |
    |     (0,0)     |
    |               |
    |               |
    |---------------|

    */
    // Arrrange
    const Mantid::Kernel::V3D origin(0, 0, 0);
    const double radius = 1;                // Not important
    const double innerBackgroundRadius = 2; // Not important
    const double outerBackgroundRadius =
        3; // This should be used to control the bounding box.
    PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                  outerBackgroundRadius);

    // Act
    const auto boundingBox = peak.getBoundingBox();

    // Assert
    auto zoomOutFactor = peak.getZoomOutFactor();
    const double expectedLeft(origin.X() -
                              zoomOutFactor * outerBackgroundRadius);
    const double expectedBottom(origin.Y() -
                                zoomOutFactor * outerBackgroundRadius);
    const double expectedRight(origin.X() +
                               zoomOutFactor * outerBackgroundRadius);
    const double expectedTop(origin.Y() +
                             zoomOutFactor * outerBackgroundRadius);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

  void test_getBoundingBox_with_offset_origin() {
    /*

    width = height = outerradius * 2
    |---------------|
    |               |
    |               |
    |     (-1,1)    |
    |               |
    |               |
    |---------------|

    */
    // Arrange
    const Mantid::Kernel::V3D origin(-1, 1, 0); // Offset origin from (0, 0, 0)
    const double radius = 1;                    // Not important
    const double innerBackgroundRadius = 2;     // Not important
    const double outerBackgroundRadius =
        3; // This should be used to control the bounding box.
    PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                  outerBackgroundRadius);

    // Act
    auto boundingBox = peak.getBoundingBox();

    // Assert
    auto zoomOutFactor = peak.getZoomOutFactor();
    const double expectedLeft(origin.X() -
                              zoomOutFactor * outerBackgroundRadius);
    const double expectedBottom(origin.Y() -
                                zoomOutFactor * outerBackgroundRadius);
    const double expectedRight(origin.X() +
                               zoomOutFactor * outerBackgroundRadius);
    const double expectedTop(origin.Y() +
                             zoomOutFactor * outerBackgroundRadius);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }
};

class PeakRepresentationSphereTestPerformance : public CxxTest::TestSuite {
public:
  /**
  Here we create a distribution of Peaks. Peaks are dispersed. This is to give
  a measurable peformance.
  */
  PeakRepresentationSphereTestPerformance() {
    const int sizeInAxis = 100;
    const double radius = 5;
    const double innerBackgroundRadius = 6;
    const double outerBackgroundRadius = 7;
    m_peaks.reserve(sizeInAxis * sizeInAxis * sizeInAxis);
    for (int x = 0; x < sizeInAxis; ++x) {
      for (int y = 0; y < sizeInAxis; ++y) {
        for (int z = 0; z < sizeInAxis; ++z) {
          Mantid::Kernel::V3D peakOrigin(x, y, z);
          m_peaks.push_back(boost::make_shared<
              PeakRepresentationSphereExposeProtectedWrapper>(
              peakOrigin, radius, innerBackgroundRadius,
              outerBackgroundRadius));
        }
      }
    }

    PeakRepresentationViewInformation viewInformation;
    viewInformation.viewHeight = 1.0;
    viewInformation.viewWidth = 1.0;
    viewInformation.windowHeight = 1.0;
    viewInformation.windowWidth = 1.0;
    viewInformation.xOriginWindow = 1;
    viewInformation.yOriginWindow = 1;

    m_viewInformation = viewInformation;
  }

  /// Test the performance of just setting the slice point.
  void test_setSlicePoint_performance() {
    for (double z = 0; z < 100; z += 5) {
      VecPeaksRepresentationSphere::iterator it = m_peaks.begin();
      while (it != m_peaks.end()) {
        auto physicalPeak = *it;
        // physicalPeak->setSlicePoint(z);
        ++it;
      }
    }
  }

  /// Test the performance of just drawing.
  void test_draw_performance() {
    const int nTimesRedrawAll = 20;
    int timesDrawn = 0;
    while (timesDrawn < nTimesRedrawAll) {
      // Set the slicing point on all peaks.
      VecPeaksRepresentationSphere::iterator it = m_peaks.begin();
      while (it != m_peaks.end()) {
        // it->getDrawingInformationWrapper(m_viewInformation);
        ++it;
      }
      ++timesDrawn;
    }
  }

  /// Test the performance of both setting the slice point and drawing..
  void test_whole_performance() {
    VecPeaksRepresentationSphere::iterator it = m_peaks.begin();
    // const double z = 10;
    while (it != m_peaks.end()) {
      // it->setSlicePoint(z);
      // it->getDrawingInformationWrapper(m_viewInformation);
      ++it;
    }
  }

private:
  using PeaksRepresentationSphere_sptr =
      boost::shared_ptr<PeakRepresentationSphereExposeProtectedWrapper>;
  using VecPeaksRepresentationSphere =
      std::vector<PeaksRepresentationSphere_sptr>;

  /// Collection to store a large number of physicalPeaks.
  VecPeaksRepresentationSphere m_peaks;
  PeakRepresentationViewInformation m_viewInformation;
};

#endif
