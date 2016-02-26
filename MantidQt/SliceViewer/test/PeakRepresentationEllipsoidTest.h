#ifndef SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_
#define SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PeakRepresentationEllipsoid.h"
#include "MockObjects.h"

using namespace MantidQt::SliceViewer;
using namespace testing;

#if 0
class PeakRepresentationEllipsoidExposeProtectedWrapper
    : public PeakRepresentationEllipsoid
{
public:
    PeakRepresentationEllipsoidExposeProtectedWrapper(
        const Mantid::Kernel::V3D &origin, const double &peakRadius,
        const double &backgroundInnerRadius,
        const double &backgroundOuterRadius)
        : PeakRepresentationSphere(origin, peakRadius, backgroundInnerRadius,
                                   backgroundOuterRadius)
    {
    }
    std::shared_ptr<PeakPrimitives> getDrawingInformationWrapper(
        PeakRepresentationViewInformation viewInformation)
    {
        return getDrawingInformation(viewInformation);
    }
};
#endif
class PeakRepresentationEllipsoidTest : public CxxTest::TestSuite
{
public:
    void test_getRadius_gets_radius_if_background_is_not_shown()
    {
        // Arrange
        std::vector<double> peakRadii{4, 3, 2};
        auto peak = providePeakRepresentation(peakRadii[0], peakRadii[1],
                                              peakRadii[2]);

        // Act + Assert
        const double delta = 1e-5;
        TSM_ASSERT_DELTA(
            "The peak radius of the major axis should be shown which is 4.",
            peakRadii[0], peak.getEffectiveRadius(), delta);
    }

    void test_getRadius_gets_outer_background_radius_if_background_is_shown()
    {
        // Arrange
        std::vector<double> peakRadii{4, 3, 2};
        // Note that the backround outer radius is incremented by 2
        auto peak = providePeakRepresentation(peakRadii[0], peakRadii[1],
                                              peakRadii[2]);

        peak.showBackgroundRadius(true);

        // Act + Assert
        const double delta = 1e-5;
        const double expectedEffectiveRadius = peakRadii[0] + 2;
        TSM_ASSERT_DELTA(
            "The peak radius of the major axis should be shown which is 4+2.",
            expectedEffectiveRadius, peak.getEffectiveRadius(), delta);
    }

    void test_handle_outer_background_radius_zero()
    {
#if 0
        // Arrange
        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius
            = 0; // This can happen using IntegratePeaksMD.
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
        auto drawingInformation
            = peak.getDrawingInformationWrapper(viewInformation);

        // Assert
        auto drawingInformationSphere
            = std::static_pointer_cast<PeakPrimitiveCircle>(
                drawingInformation);
        // The Return object should be initialized to zero in every field.
        TS_ASSERT_EQUALS(drawingInformationSphere->backgroundOuterRadiusX,
                         drawingInformationSphere->backgroundInnerRadiusX);
        TS_ASSERT_EQUALS(drawingInformationSphere->backgroundOuterRadiusY,
                         drawingInformationSphere->backgroundInnerRadiusY);
#endif
    }

    void
    test_that_setting_slice_point_to_intersect_produces_valid_drawing_information()
    {
#if 0
        // Arrange
        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius = 3;
        PeakRepresentationSphereExposeProtectedWrapper peak(
            origin, radius, innerBackgroundRadius, outerBackgroundRadius);

        const double slicePoint = radius
                                  / 2; // set to be half way through the radius.
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
        auto drawingInformation
            = peak.getDrawingInformationWrapper(viewInformation);

        // Assert
        auto drawingInformationSphere
            = std::static_pointer_cast<PeakPrimitiveCircle>(
                drawingInformation);

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
#endif
    }

    void test_move_position_produces_correct_position()
    {
#if 0
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
#endif
    }

    void test_getBoundingBox()
    {
#if 0
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
        const double outerBackgroundRadius
            = 3; // This should be used to control the bounding box.
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        // Act
        const auto boundingBox = peak.getBoundingBox();

        // Assert
        const double expectedLeft(origin.X() - outerBackgroundRadius);
        const double expectedBottom(origin.Y() - outerBackgroundRadius);
        const double expectedRight(origin.X() + outerBackgroundRadius);
        const double expectedTop(origin.Y() + outerBackgroundRadius);

        TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
        TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
        TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
        TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
#endif
    }

    void test_getBoundingBox_with_offset_origin()
    {
#if 0
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
        const Mantid::Kernel::V3D origin(-1, 1,
                                         0);    // Offset origin from (0, 0, 0)
        const double radius = 1;                // Not important
        const double innerBackgroundRadius = 2; // Not important
        const double outerBackgroundRadius
            = 3; // This should be used to control the bounding box.
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        // Act
        auto boundingBox = peak.getBoundingBox();

        // Assert
        const double expectedLeft(origin.X() - outerBackgroundRadius);
        const double expectedBottom(origin.Y() - outerBackgroundRadius);
        const double expectedRight(origin.X() + outerBackgroundRadius);
        const double expectedTop(origin.Y() + outerBackgroundRadius);

        TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
        TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
        TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
        TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
#endif
    }

private:
    PeakRepresentationEllipsoid providePeakRepresentation(double r1, double r2,
                                                          double r3)
    {
        Mantid::Kernel::V3D origin(0, 0, 0);
        std::vector<double> peakRadii{r1, r2, r3};
        std::vector<double> backgroundRadiiInner{r1 + 1, r2 + 1, r3 + 1};
        std::vector<double> backgroundRadiiOuter{r1 + 2, r2 + 2, r3 + 2};

        Mantid::Kernel::V3D dir1{1, 0, 0};
        Mantid::Kernel::V3D dir2{0, 1, 0};
        Mantid::Kernel::V3D dir3{0, 0, 1};
        std::vector<Mantid::Kernel::V3D> directions = {dir1, dir2, dir3};

        auto calculator = std::
            make_shared<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>();

        return PeakRepresentationEllipsoid(
            origin, peakRadii, backgroundRadiiInner, backgroundRadiiOuter,
            directions, calculator);
    }
};

#endif
