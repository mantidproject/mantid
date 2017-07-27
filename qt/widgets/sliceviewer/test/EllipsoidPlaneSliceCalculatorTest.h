#ifndef SLICE_VIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_TEST_H_
#define SLICE_VIEWER_ELLIPSOID_PLANE_SLICE_CALCULATOR_TEST_H_

#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidQtSliceViewer/EllipsoidPlaneSliceCalculator.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <math.h>
#include <unordered_map>

namespace {
bool radiusIsInListOfRadii(double radius, const std::vector<double> &radii) {
  auto comparison = [radius](double toCheck) {
    return Mantid::SliceViewer::almost_equal(radius, toCheck);
  };
  return std::any_of(radii.cbegin(), radii.cend(), comparison);
}

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
}

class EllipsoidPlaneSliceCalculatorTest : public CxxTest::TestSuite {
public:
  //-----------------------------------------------------------------
  // Test for the creation of the Ellipsoid Matrix
  //-----------------------------------------------------------------
  void
  test_that_correct_ellipsoid_matrix_is_generated_when_already_in_eigenbasis() {
    // Arrange
    std::vector<double> radii = {3.0, 2.0, 1.0};
    Mantid::Kernel::V3D dir1(1.0, 0.0, 0.0);
    Mantid::Kernel::V3D dir2(0.0, 1.0, 0.0);
    Mantid::Kernel::V3D dir3(0.0, 0.0, 1.0);
    std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};

    // Act
    auto matrix =
        Mantid::SliceViewer::createEllipsoidMatrixInXYZFrame(directions, radii);

    // Assert

    std::vector<std::pair<int, int>> indices = {
        {0, 1}, {0, 2}, {1, 0}, {1, 2}, {2, 0}, {2, 1}};

    for (const auto &index : indices) {
      TSM_ASSERT("Non-diagonal element should be zero",
                 matrix[index.first][index.second] == 0.0);
    }
    TSM_ASSERT_EQUALS("Should be the first 1/radius^2", matrix[0][0],
                      1 / std::pow(radii[0], 2));
    TSM_ASSERT_EQUALS("Should be the second 1/radius^2", matrix[1][1],
                      1 / std::pow(radii[1], 2));
    TSM_ASSERT_EQUALS("Should be the third 1/radius^2", matrix[2][2],
                      1 / std::pow(radii[2], 2));
  }

  void
  test_that_correct_ellipsoid_matrix_is_generated_when_eigenbasis_rotated_45_degrees_around_z() {

    // Arrange
    std::vector<double> radii = {3.0, 2.0, 1.5};
    std::vector<double> inverseRadiiSquared = {1 / std::pow(radii[0], 2),
                                               1 / std::pow(radii[1], 2),
                                               1 / std::pow(radii[2], 2)};
    Mantid::Kernel::V3D dir1(M_SQRT1_2, -1 * M_SQRT1_2, 0.0);
    Mantid::Kernel::V3D dir2(M_SQRT1_2, M_SQRT1_2, 0.0);
    Mantid::Kernel::V3D dir3(0.0, 0.0, 1.0);
    std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};

    // Act
    auto matrix =
        Mantid::SliceViewer::createEllipsoidMatrixInXYZFrame(directions, radii);

    // Assert
    Mantid::Kernel::DblMatrix eigenValues; // on the diagonal
    Mantid::Kernel::DblMatrix eigenVectors;
    matrix.Diagonalise(eigenVectors, eigenValues);

    double delta = 1e-5;
    for (int index = 0; index < 3; ++index) {
      auto invRad = inverseRadiiSquared[index];
      TSM_ASSERT_DELTA("Eigenvalue should correspond to inverse radius squared",
                       invRad, eigenValues[index][index], delta);
    }
  }

  //-----------------------------------------------------------------
  // Tests for handling spheres
  //-----------------------------------------------------------------
  void
  test_that_correct_slice_information_is_generated_for_sphere_with_cut_through_origin_and_origin_at_0() {

    // Arrange
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;
    double zCutPlane = 0; // We cut at z = 0
    Mantid::Kernel::V3D direction1(1, 0,
                                   0); // The directions are simply x, y, z
    Mantid::Kernel::V3D direction2(0, 1, 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {
        1.0, 1.0, 1.0}; // The radii are equal, hence we have a sphere
    Mantid::Kernel::V3D origin(0.0, 0.0, 0.0); // The origin is at 0,0,0

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;
    TSM_ASSERT("The angle should be 0",
               isAngleEitherValueOr180DegreesRoated(0.0, info.angle));

    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, radii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, radii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 0", info.origin.X(),
                     0.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 0", info.origin.Y(),
                     0.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 0", info.origin.Z(),
                     0.0, delta);
  }

  void
  test_that_correct_slice_information_is_generated_for_sphere_with_cut_through_1_and_origin_at__3_2_1() {
    // Arrange
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;
    double zCutPlane = 1; // We cut at z = 1
    Mantid::Kernel::V3D direction1(1, 0,
                                   0); // The directions are simply x, y, z
    Mantid::Kernel::V3D direction2(0, 1, 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {
        1.0, 1.0, 1.0}; // The radii are equal, hence we have a sphere
    Mantid::Kernel::V3D origin(3.0, 2.0, 1.0); // The origin is at 3,2,1

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;
    TSM_ASSERT("The angle should be 0",
               isAngleEitherValueOr180DegreesRoated(0.0, info.angle));
    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, radii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMinorAxis, radii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 3", info.origin.X(),
                     3.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 2", info.origin.Y(),
                     2.0, delta);

    TSM_ASSERT_DELTA("The z part of the origin should be at 1", info.origin.Z(),
                     1.0, delta);
  }

  void
  test_that_correct_slice_information_is_generated_for_sphere_with_cut_through_1_5_and_origin_at__3_2_1() {
    // Arrange
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;
    double zCutPlane = 1.5; // We cut at z = 1.5
    Mantid::Kernel::V3D direction1(1, 0,
                                   0); // The directions are simply x, y, z
    Mantid::Kernel::V3D direction2(0, 1, 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {
        1.0, 1.0, 1.0}; // The radii are equal, hence we have a sphere
    Mantid::Kernel::V3D origin(3.0, 2.0, 1.0); // The origin is at 3,2,1

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;
    TSM_ASSERT("The angle should be 0",
               isAngleEitherValueOr180DegreesRoated(0.0, info.angle));

    // Radius is 1 and we are looking at 0.5 from the origin
    // ie x^2 + y^2 + 0.5^2 == r^2 ==> reffective^2 = 1^2 -0.5^2 = 0.75
    std::vector<double> expcetedRadii = {std::sqrt(0.75)};
    TSM_ASSERT("The first radius should be Sqrt[0.75]",
               radiusIsInListOfRadii(info.radiusMajorAxis, expcetedRadii));
    TSM_ASSERT("The second radius should be Sqrt[0.75]",
               radiusIsInListOfRadii(info.radiusMinorAxis, expcetedRadii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 3", info.origin.X(),
                     3.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 2", info.origin.Y(),
                     2.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 2", info.origin.Z(),
                     1.5, delta);
  }

  void
  test_that_correct_slice_information_is_generated_for_sphere_with_cut_through_1_5_and_origin_at__3_2_1_with_tilted_axes() {
    // Arrange
    const double angleIn = 30 / 180 * M_PI;
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;
    double zCutPlane = 1.5; // We cut at z = 1.5
    Mantid::Kernel::V3D direction1(std::cos(angleIn), std::sin(angleIn),
                                   0); // The directions are simply x, y, z
    Mantid::Kernel::V3D direction2(-std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {
        1.0, 1.0, 1.0}; // The radii are equal, hence we have a sphere
    Mantid::Kernel::V3D origin(3.0, 2.0, 1.0); // The origin is at 3,2,1

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;

    // The rotation was 30deg, but we have a sphere so angle should be 0
    TSM_ASSERT("The angle should be 0",
               isAngleEitherValueOr180DegreesRoated(0.0, info.angle));

    // Radius is 1 and we are looking at 0.5 from the origin
    // ie x^2 + y^2 + 0.5^2 == r^2 ==> reffective^2 = 1^2 -0.5^2 = 0.75
    std::vector<double> expcetedRadii = {std::sqrt(0.75)};
    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, expcetedRadii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMinorAxis, expcetedRadii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 3", info.origin.X(),
                     3.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 2", info.origin.Y(),
                     2.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 2", info.origin.Z(),
                     1.5, delta);
  }

  //-----------------------------------------------------------------
  // Tests for handling ellipsoids
  //-----------------------------------------------------------------
  void
  test_correct_for_ellipsoid_with_cut_through_0_and_origin_at_0_with_axis_aligned() {
    // Arrange
    const double angleIn = 0.0;
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;
    double zCutPlane = 0;
    Mantid::Kernel::V3D direction1(std::cos(angleIn), -std::sin(angleIn), 0);
    Mantid::Kernel::V3D direction2(std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {4.0, 3.0, 2.0};
    Mantid::Kernel::V3D origin(0.0, 0.0, 0.0);

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;

    TSM_ASSERT("The angle should be 0",
               isAngleEitherValueOr180DegreesRoated(0.0, info.angle));

    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, radii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMinorAxis, radii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 0", info.origin.X(),
                     0.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 0", info.origin.Y(),
                     0.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 0", info.origin.Z(),
                     0.0, delta);
  }

  void
  test_correct_for_ellipsoid_with_cut_through_1_5_and_origin_at_3_2_1_with_axis_aligned() {
    // Arrange
    const double angleIn = 0.0;
    double zCutPlane = 1.5;
    Mantid::Kernel::V3D direction1(std::cos(angleIn), -std::sin(angleIn), 0);
    Mantid::Kernel::V3D direction2(std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {4.0, 3.0, 2.0};
    Mantid::Kernel::V3D origin(3.0, 2.0, 1.0);

    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;

    TSM_ASSERT("The angle should be 0",
               isAngleEitherValueOr180DegreesRoated(0.0, info.angle));

    // From (x/4)^2 + (y/3)^2 + (0.5/2)^2 = 1 we get
    // r1 = 4* Sqrt[1-(0.5/2)^2]
    // r2 = 3* Sqrt[1-(0.5/2)^2]
    std::vector<double> expectedRadii = {4.0 * std::sqrt(1 - std::pow(0.25, 2)),
                                         3.0 *
                                             std::sqrt(1 - std::pow(0.25, 2))};

    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, expectedRadii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMinorAxis, expectedRadii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 3", info.origin.X(),
                     3.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 2", info.origin.Y(),
                     2.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 1.5",
                     info.origin.Z(), 1.5, delta);
  }

  void
  test_correct_for_ellipsoid_with_cut_through_1_5_and_origin_at_3_2_1_with_axis_tilt_with_less_than_45_degrees() {
    double const angle = 32;         // in degree
    double const expectedAngle = 32; // in degree
    do_test_ellipsoid_with_tilt(angle, expectedAngle);
  }

  void
  test_correct_for_ellipsoid_with_cut_through_1_5_and_origin_at_3_2_1_with_axis_tilt_with_45_degrees() {
    double const angle = 45;         // in degree
    double const expectedAngle = 45; // in degree
    do_test_ellipsoid_with_tilt(angle, expectedAngle);
  }

  void
  test_correct_for_ellipsoid_with_cut_through_1_5_and_origin_at_3_2_1_with_axis_tilt_with_more_than_45_degrees() {
    double const angle = 48;         // in degree
    double const expectedAngle = 48; // in degree
    do_test_ellipsoid_with_tilt(angle, expectedAngle);
  }

  void
  test_finds_ellipse_for_ellipsoid_with_major_axis_along_z_and_cut_through_5_and_origin_at_3_2_1_with_axis_tilt() {
    // Arrange
    const double angleIn = 34.0 * 2 * M_PI / 360;
    double zCutPlane = 5.0;
    Mantid::Kernel::V3D direction1(std::cos(angleIn), std::sin(angleIn), 0);
    Mantid::Kernel::V3D direction2(-std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {4.0, 3.0, 7.0};
    Mantid::Kernel::V3D origin(3.0, 2.0, 1.0);

    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;

    // The angle we get from info is the angle
    TSM_ASSERT("The angle should be 34",
               isAngleEitherValueOr180DegreesRoated(angleIn, info.angle));

    // From (x/4)^2 + (y/3)^2 + ((5-1)/7)^2 = 1 we get
    // r1 = 4* Sqrt[1-(4/7)^2]
    // r2 = 3* Sqrt[1-(4/7)^2]
    std::vector<double> expectedRadii = {
        4.0 * std::sqrt(1 - std::pow(4.0 / 7.0, 2)),
        3.0 * std::sqrt(1 - std::pow(4.0 / 7.0, 2))};

    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, expectedRadii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMinorAxis, expectedRadii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 3", info.origin.X(),
                     3.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 2", info.origin.Y(),
                     2.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 5", info.origin.Z(),
                     5, delta);
  }

  //-----------------------------------------------------------------
  // Tests for bounding box of an ellipse
  //-----------------------------------------------------------------
  void test_that_bounding_box_of_axis_aligned_ellipsoid_is_found() {
    // Arrange
    const double angleIn = 0.0;
    Mantid::Kernel::V3D direction1(std::cos(angleIn), std::sin(angleIn), 0);
    Mantid::Kernel::V3D direction2(-std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    Mantid::Kernel::V3D origin(1.0, 2.0, -1.0);
    std::vector<double> radii = {2.0, 1.5, 0.5};

    // Act
    auto boundingBox = Mantid::SliceViewer::getPeakBoundingBoxForEllipsoid(
        directions, radii, origin);

    // Assert
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calc;
    const auto zoomOutFactor = calc.getZoomOutFactor();
    const double expectedLeft = origin[0] - zoomOutFactor * radii[0];
    const double expectedRight = origin[0] + zoomOutFactor * radii[0];
    const double expectedTop = origin[1] + zoomOutFactor * radii[1];
    const double expectedBottom = origin[1] - zoomOutFactor * radii[1];
    const double expectedSlicePoint = origin[2];

    const double delta = 1e-5;
    TSM_ASSERT_DELTA("Left should be at -1.0.", expectedLeft,
                     boundingBox.left(), delta);
    TSM_ASSERT_DELTA("Right should be at 3.0.", expectedRight,
                     boundingBox.right(), delta);
    TSM_ASSERT_DELTA("Top should be at 3.5.", expectedTop, boundingBox.top(),
                     delta);
    TSM_ASSERT_DELTA("Bottom should be at 0.5.", expectedBottom,
                     boundingBox.bottom(), delta);
    TSM_ASSERT_DELTA("SlicePoint should be at -1.0.", expectedSlicePoint,
                     boundingBox.slicePoint(), delta);
  }

  void test_that_bounding_box_of_non_axis_aligned_ellipsoid_is_found() {
    // Arrange
    const double angleIn = 20.0 * M_PI / 180.0;
    Mantid::Kernel::V3D direction1(std::cos(angleIn), std::sin(angleIn), 0);
    Mantid::Kernel::V3D direction2(-std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    Mantid::Kernel::V3D origin(3.0, 1.0, 0.0);
    std::vector<double> radii = {2.0, 1.0, 0.5};

    // Act
    auto boundingBox = Mantid::SliceViewer::getPeakBoundingBoxForEllipsoid(
        directions, radii, origin);

    // Assert
    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calc;
    const auto zoomOutFactor = calc.getZoomOutFactor();
    const double expectedLeft =
        origin[0] - zoomOutFactor * radii[0] * std::cos(angleIn);
    const double expectedRight =
        origin[0] + zoomOutFactor * radii[0] * std::cos(angleIn);
    const double expectedTop =
        origin[1] + zoomOutFactor * radii[1] * std::cos(angleIn);
    const double expectedBottom =
        origin[1] - zoomOutFactor * radii[1] * std::cos(angleIn);
    const double expectedSlicePoint = origin[2];

    const double delta = 1e-5;
    TSM_ASSERT_DELTA("Left should be at -2.0*Cos[angle] +3.", expectedLeft,
                     boundingBox.left(), delta);
    TSM_ASSERT_DELTA("Right should be at 2.0*Cos[angle] +3 .", expectedRight,
                     boundingBox.right(), delta);
    TSM_ASSERT_DELTA("Top should be at 1.0*Cos[angle].", expectedTop,
                     boundingBox.top(), delta);
    TSM_ASSERT_DELTA("Bottom should be at -1.0*Cos[angle].", expectedBottom,
                     boundingBox.bottom(), delta);
    TSM_ASSERT_DELTA("SlicePoint should be at 0.0.", expectedSlicePoint,
                     boundingBox.slicePoint(), delta);
  }

  //-----------------------------------------------------------------
  // Tests for finding projection of the ellipsoid onto xyz axes
  //-----------------------------------------------------------------
  void test_that_projection_for_axis_aligned_is_found() {
    // Arrange
    Mantid::Kernel::V3D dir1(1.0, 0.0, 0.0);
    Mantid::Kernel::V3D dir2(0.0, 1.0, 0.0);
    Mantid::Kernel::V3D dir3(0.0, 0.0, 1.0);
    std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};
    std::vector<double> radii = {3.0, 2.0, 1.0};

    // Act
    auto projections =
        Mantid::SliceViewer::getProjectionLengths(directions, radii);

    // Assert
    const double delta = 1e-5;
    TSM_ASSERT_DELTA("Projection onto x should be 3.", projections[0], radii[0],
                     delta);
    TSM_ASSERT_DELTA("Projection onto y should be 2.", projections[1], radii[1],
                     delta);
    TSM_ASSERT_DELTA("Projection onto y should be 1.", projections[2], radii[2],
                     delta);
  }

  void test_that_projection_for_non_axis_aligned_is_found() {
    // Arrange
    const double angle = 35.0 * M_PI / 180.0;
    Mantid::Kernel::V3D dir1(std::cos(angle), std::sin(angle), 0.0);
    Mantid::Kernel::V3D dir2(-std::sin(angle), std::cos(angle), 0.0);
    Mantid::Kernel::V3D dir3(0.0, 0.0, 1.0);
    std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};
    std::vector<double> radii = {3.0, 2.0, 1.0};

    // Act
    auto projections =
        Mantid::SliceViewer::getProjectionLengths(directions, radii);

    // Assert
    auto expectedProjetionX = dir1[0] * radii[0];
    auto expectedProjectionY =
        dir1[1] * radii[0]; // Note that the first direction dominates the
                            // projection on both x and y in this
                            // particular case

    const double delta = 1e-5;
    TSM_ASSERT_DELTA("Projection onto x should be 3.", projections[0],
                     expectedProjetionX, delta);
    TSM_ASSERT_DELTA("Projection onto y should be 2.", projections[1],
                     expectedProjectionY, delta);
    TSM_ASSERT_DELTA("Projection onto y should be 1.", projections[2], radii[2],
                     delta);
  }

  //-----------------------------------------------------------------
  // Tests for checking if a cut exists
  //-----------------------------------------------------------------
  void test_that_that_detects_if_cut_exists() {
    // Arrange
    const double angle = 35.0 * M_PI / 180.0;
    Mantid::Kernel::V3D dir1(std::cos(angle), std::sin(angle), 0.0);
    Mantid::Kernel::V3D dir2(-std::sin(angle), std::cos(angle), 0.0);
    Mantid::Kernel::V3D dir3(0.0, 0.0, 1.0);
    std::vector<Mantid::Kernel::V3D> directions{dir1, dir2, dir3};
    std::vector<double> radii = {3.0, 2.0, 1.0};
    Mantid::Kernel::V3D originEllipsoid(2.0, 6.0, 3.0);
    double zPlane = 3.7;

    // Act
    auto cutExists = Mantid::SliceViewer::checkIfCutExists(
        directions, radii, originEllipsoid, zPlane);

    // Assert
    TSM_ASSERT("Cut should be possible", cutExists);
  }

  void test_that_that_detects_if_cut_does_not_exist() {
    // Arrange
    const double angle = 35.0 * M_PI / 180.0;
    Mantid::Kernel::V3D dir1(std::cos(angle), std::sin(angle), 0.0);
    Mantid::Kernel::V3D dir2(-std::sin(angle), std::cos(angle), 0.0);
    Mantid::Kernel::V3D dir3(0.0, 0.0, 1.0);
    std::vector<Mantid::Kernel::V3D> directions{dir1, dir2, dir3};
    std::vector<double> radii = {3.0, 2.0, 1.0};
    Mantid::Kernel::V3D originEllipsoid(2.0, 6.0, 3.0);
    double zPlane = 4.5;

    // Act
    auto cutExists = Mantid::SliceViewer::checkIfCutExists(
        directions, radii, originEllipsoid, zPlane);

    // Assert
    TSM_ASSERT("Cut should not be possible", !cutExists);
  }

private:
  void do_test_ellipsoid_with_tilt(double angle, double expectedAngle) {
    // Arrange
    const double angleIn = angle * 2 * M_PI / 360.0;
    double zCutPlane = 1.5;
    Mantid::Kernel::V3D direction1(std::cos(angleIn), std::sin(angleIn), 0);
    Mantid::Kernel::V3D direction2(-std::sin(angleIn), std::cos(angleIn), 0);
    Mantid::Kernel::V3D direction3(0, 0, 1);
    std::vector<Mantid::Kernel::V3D> directions = {direction1, direction2,
                                                   direction3};
    std::vector<double> radii = {4.0, 3.0, 2.0};
    Mantid::Kernel::V3D origin(3.0, 2.0, 1.0);

    Mantid::SliceViewer::EllipsoidPlaneSliceCalculator calculator;

    // Act
    auto info =
        calculator.getSlicePlaneInfo(directions, radii, origin, zCutPlane);

    // Assert
    const double delta = 1e-5;

    // The angle we get from info is the angle
    TSM_ASSERT("The angle should be be equal",
               isAngleEitherValueOr180DegreesRoated(
                   expectedAngle * M_PI / 180.0, info.angle));

    // From (x/4)^2 + (y/3)^2 + (0.5/2)^2 = 1 we get
    // r1 = 4* Sqrt[1-(0.5/2)^2]
    // r2 = 3* Sqrt[1-(0.5/2)^2]
    std::vector<double> expectedRadii = {4.0 * std::sqrt(1 - std::pow(0.25, 2)),
                                         3.0 *
                                             std::sqrt(1 - std::pow(0.25, 2))};

    TSM_ASSERT("The first radius should be 1",
               radiusIsInListOfRadii(info.radiusMajorAxis, expectedRadii));
    TSM_ASSERT("The second radius should be 1",
               radiusIsInListOfRadii(info.radiusMinorAxis, expectedRadii));

    TSM_ASSERT_DELTA("The x part of the origin should be at 3", info.origin.X(),
                     3.0, delta);
    TSM_ASSERT_DELTA("The y part of the origin should be at 2", info.origin.Y(),
                     2.0, delta);
    TSM_ASSERT_DELTA("The z part of the origin should be at 1.5",
                     info.origin.Z(), 1.5, delta);
  }
};

#endif
