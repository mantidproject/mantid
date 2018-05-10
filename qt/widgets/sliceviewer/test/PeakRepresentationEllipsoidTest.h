#ifndef SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_
#define SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_

#include "MantidQtWidgets/SliceViewer/PeakRepresentationEllipsoid.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::SliceViewer;
using namespace testing;

namespace {
// Check if the two angles are the same, note that angles which are shifted by
// 180 degrees
// are the same for an ellipsoid, ie one is free to have the major axis point in
// the
// + or - direction
bool isAngleEitherValueOr180DegreesRoated(double expectedAngle,
                                          double actualAngle) {
  return Mantid::SliceViewer::almost_equal(expectedAngle, actualAngle) ||
         Mantid::SliceViewer::almost_equal(expectedAngle, actualAngle + M_PI) ||
         Mantid::SliceViewer::almost_equal(expectedAngle, actualAngle - M_PI);
}
} // namespace

class PeakRepresentationEllipsoidExposeProtectedWrapper
    : public PeakRepresentationEllipsoid {
public:
  PeakRepresentationEllipsoidExposeProtectedWrapper(
      const Mantid::Kernel::V3D &origin, const std::vector<double> &peakRadius,
      const std::vector<double> &backgroundInnerRadius,
      const std::vector<double> &backgroundOuterRadius,
      const std::vector<Mantid::Kernel::V3D> directions,
      std::shared_ptr<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>
          calculator)
      : PeakRepresentationEllipsoid(origin, peakRadius, backgroundInnerRadius,
                                    backgroundOuterRadius, directions,
                                    calculator)

  {}
  std::shared_ptr<PeakPrimitives> getDrawingInformationWrapper(
      PeakRepresentationViewInformation viewInformation) {
    return getDrawingInformation(viewInformation);
  }
};

class PeakRepresentationEllipsoidTest : public CxxTest::TestSuite {
public:
  void test_getRadius_gets_radius_if_background_is_not_shown() {
    // Arrange
    std::vector<double> peakRadii{4, 3, 2};
    auto peak =
        providePeakRepresentation(peakRadii[0], peakRadii[1], peakRadii[2]);

    // Act + Assert
    const double delta = 1e-5;
    TSM_ASSERT_DELTA(
        "The peak radius of the major axis should be shown which is 4.",
        peakRadii[0], peak.getEffectiveRadius(), delta);
  }

  void test_getRadius_gets_outer_background_radius_if_background_is_shown() {
    // Arrange
    std::vector<double> peakRadii{4, 3, 2};
    // Note that the backround outer radius is incremented by 2
    auto peak =
        providePeakRepresentation(peakRadii[0], peakRadii[1], peakRadii[2]);
    peak.showBackgroundRadius(true);

    // Act + Assert
    const double delta = 1e-5;
    const double expectedEffectiveRadius = peakRadii[0] + 2;
    TSM_ASSERT_DELTA(
        "The peak radius of the major axis should be shown which is 4+2.",
        expectedEffectiveRadius, peak.getEffectiveRadius(), delta);
  }

  void
  test_that_setting_slice_point_to_intersect_produces_valid_drawing_information() {
    // Arrange
    const double r1 = 5;
    const double r2 = 4;
    const double r3 = 3;
    const double angle = 35.0 * M_PI / 180.0;
    auto peak = providePeakRepresentationWrapper(r1, r2, r3, angle);

    const double slicePoint = r3 / 2; // set to be half way through the radius.
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
    auto drawingInformationEllipse =
        std::static_pointer_cast<PeakPrimitivesEllipse>(drawingInformation);
    const double delta = 1e-5;
    ;
    const double expectedOpacityAtDistance = 0.8 * (1 - 1.5 / (3 + 2));
    const double expectedAngle = angle;
    TS_ASSERT_DELTA(expectedOpacityAtDistance,
                    drawingInformationEllipse->peakOpacityAtDistance, delta);
    TSM_ASSERT("Should have an angle of 35.",
               isAngleEitherValueOr180DegreesRoated(
                   expectedAngle, drawingInformationEllipse->angle));
  }

  void test_getBoundingBox() {
    // Arrange
    const double r1 = 5;
    const double r2 = 4;
    const double r3 = 3;
    const double angle = 32;
    auto peak = providePeakRepresentation(r1, r2, r3, angle);

    // Act
    const auto boundingBox = peak.getBoundingBox();

    // Assert
    // Looking at background radius, hence + 2
    auto zoomOutFactor = peak.getZoomOutFactor();
    const double expectedLeft(0 - zoomOutFactor * (std::cos(angle) * (r1 + 2)));
    const double expectedRight(0 +
                               zoomOutFactor * (std::cos(angle) * (r1 + 2)));

    const double expectedBottom(0 -
                                zoomOutFactor * (std::cos(angle) * (r2 + 2)));
    const double expectedTop(0 + zoomOutFactor * (std::cos(angle) * (r2 + 2)));

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

  void test_getBoundingBox_with_offset_origin() {
    // Arrange
    const double originX = 1;
    const double originY = 2;
    const double originZ = 1;
    const double r1 = 5;
    const double r2 = 4;
    const double r3 = 3;
    const double angle = 32;
    auto peak =
        providePeakRepresentation(r1, r2, r3, angle, originX, originY, originZ);

    // Act
    const auto boundingBox = peak.getBoundingBox();

    // Assert
    // Looking at background radius, hence + 2
    auto zoomOutFactor = peak.getZoomOutFactor();
    const double expectedLeft(0 - zoomOutFactor * (std::cos(angle) * (r1 + 2)) +
                              originX);
    const double expectedRight(
        0 + zoomOutFactor * (std::cos(angle) * (r1 + 2)) + originX);

    const double expectedBottom(
        0 - zoomOutFactor * (std::cos(angle) * (r2 + 2)) + originY);
    const double expectedTop(0 + zoomOutFactor * (std::cos(angle) * (r2 + 2)) +
                             originY);

    TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
    TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
    TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
    TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
  }

private:
  PeakRepresentationEllipsoid
  providePeakRepresentation(double r1, double r2, double r3, double angle = 0,
                            double originX = 0, double originY = 0,
                            double originZ = 0) {
    Mantid::Kernel::V3D origin(originX, originY, originZ);
    std::vector<double> peakRadii{r1, r2, r3};
    std::vector<double> backgroundRadiiInner{r1 + 1, r2 + 1, r3 + 1};
    std::vector<double> backgroundRadiiOuter{r1 + 2, r2 + 2, r3 + 2};

    Mantid::Kernel::V3D dir1{std::cos(angle), std::sin(angle), 0};
    Mantid::Kernel::V3D dir2{-std::sin(angle), std::cos(angle), 0};
    Mantid::Kernel::V3D dir3{0, 0, 1};
    std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};

    auto calculator =
        std::make_shared<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>();

    return PeakRepresentationEllipsoid(origin, peakRadii, backgroundRadiiInner,
                                       backgroundRadiiOuter, directions,
                                       calculator);
  }

  PeakRepresentationEllipsoidExposeProtectedWrapper
  providePeakRepresentationWrapper(double r1, double r2, double r3,
                                   double angle) {
    Mantid::Kernel::V3D origin(0, 0, 0);
    std::vector<double> peakRadii{r1, r2, r3};
    std::vector<double> backgroundRadiiInner{r1 + 1, r2 + 1, r3 + 1};
    std::vector<double> backgroundRadiiOuter{r1 + 2, r2 + 2, r3 + 2};

    Mantid::Kernel::V3D dir1{std::cos(angle), std::sin(angle), 0};
    Mantid::Kernel::V3D dir2{-std::sin(angle), std::cos(angle), 0};
    Mantid::Kernel::V3D dir3{0, 0, 1};
    std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};

    auto calculator =
        std::make_shared<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>();

    return PeakRepresentationEllipsoidExposeProtectedWrapper(
        origin, peakRadii, backgroundRadiiInner, backgroundRadiiOuter,
        directions, calculator);
  }
};

#endif
