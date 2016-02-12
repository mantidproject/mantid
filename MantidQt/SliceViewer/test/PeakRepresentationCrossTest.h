#ifndef SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_
#define SLICE_VIEWER_PEAK_REPRESENTATION_CROSS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PeakRepresentationCross.h"
#include "MockObjects.h"

using namespace MantidQt::SliceViewer;
using namespace testing;

class PeakRepresentationCrossTest : public CxxTest::TestSuite
{
public:
    void test_setSlicePoint_to_intersect()
    {

      // TODO CHeck what can be tested here
#if 0
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

        TS_ASSERT_EQUALS(expectedHalfCrossWidth, drawObject.peakHalfCrossWidth);
        TS_ASSERT_EQUALS(expectedHalfCrossHeight,
                         drawObject.peakHalfCrossHeight);
        TS_ASSERT_EQUALS(expectedLineWidth, drawObject.peakLineWidth);
#endif
    }

    void test_movePosition_moves_the_peak()
    {

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

    void test_getBoundingBox_gets_correct_box_without_offset_from_origin()
    {
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

    void test_getBoundingBox_gets_correct_box_with_offset_from_origin()
    {
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

    void test_expand_peak_intoplane()
    {
        // Arrange
        Mantid::Kernel::V3D origin(0, 0, 0);
        const double maxZ = 1;
        const double minZ = 0;
        PeakRepresentationCross peak(origin, maxZ, minZ);

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

    void test_expand_peak_inplane()
    {
        // Arrange
        Mantid::Kernel::V3D origin(0, 0, 0);
        const double maxZ = 1;
        const double minZ = 0;
        PeakRepresentationCross peak(origin, maxZ, minZ);

        const double occupancyFraction = 0.01; // 1%

        // Act
        peak.setOccupancyInView(occupancyFraction); // 1 %
        auto updatedOccupancyInView = peak.getOccupancyInView();

        // Assert
        TS_ASSERT_EQUALS(occupancyFraction, updatedOccupancyInView);
    }

    void test_setOccupanyIntoView_ignores_zeros()
    {
        // Act
        Mantid::Kernel::V3D origin(0, 0, 0);
        const double maxZ = 1;
        const double minZ = 0;
        PeakRepresentationCross peak(origin, maxZ, minZ);

        // Act
        double defaultOccupancy = peak.getOccupancyIntoView();
        peak.setOccupancyIntoView(0);

        // Assert
        TSM_ASSERT_DIFFERS("Should have ignored the zero value input", 0,
                           peak.getOccupancyIntoView());
        TS_ASSERT_EQUALS(defaultOccupancy, peak.getOccupancyIntoView());
    }
};

// TODO need to provide Mock QwtPlot etc to have this run properly, might be a
// bit much (and not realistic)

class PeakRepresentationCrossTestPerformance : public CxxTest::TestSuite
{
private:
    typedef std::vector<boost::shared_ptr<PeakRepresentationCross>>
        VecPeakRepCross;

    /// Collection to store a large number of PeakRepresentationCross.
    VecPeakRepCross m_peaks;

public:
    /**
    Here we create a distribution of Peaks. Peaks are dispersed. This is to give
    a measurable peformance.
    */
    PeakRepresentationCrossTestPerformance()
    {
#if 0
        const int sizeInAxis = 100;
        const double maxZ = 100;
        const double minZ = 0;
        m_peaks.reserve(sizeInAxis * sizeInAxis * sizeInAxis);
        for (int x = 0; x < sizeInAxis; ++x) {
            for (int y = 0; y < sizeInAxis; ++y) {
                for (int z = 0; z < sizeInAxis; ++z) {
                    V3D peakOrigin(x, y, z);
                    m_peaks.push_back(
                        boost::make_shared<PeakRepresentationCross>(
                            peakOrigin, maxZ, minZ));
                }
            }
        }
#endif
    }

    /// Test the performance of just setting the slice point.
    void test_setSlicePoint_performance()
    {
#if 0
        for (double z = 0; z < 100; z += 5) {
            VecPeakRepCross::iterator it = m_peaks.begin();
            while (it != m_peaks.end()) {
                (*it)->setSlicePoint(z);
                ++it;
            }
        }
#endif
    }

    /// Test the performance of just drawing.
    void test_draw_performance()
    {
#if 0
        const int nTimesRedrawAll = 20;
        int timesDrawn = 0;
        while (timesDrawn < nTimesRedrawAll) {
            auto it = m_peaks.begin();
            while (it != m_peaks.end()) {
                (*it)->draw(1, 1);
                ++it;
            }
            ++timesDrawn;
        }
#endif
    }

    /// Test the performance of both setting the slice point and drawing..
    void test_whole_performance()
    {
#if 0
        auto it = m_peaks.begin();
        const double z = 10;
        while (it != m_peaks.end()) {
            (*it)->setSlicePoint(z);
            (*it)->draw(1, 1);
            ++it;
        }

#endif
    }
};

#endif
