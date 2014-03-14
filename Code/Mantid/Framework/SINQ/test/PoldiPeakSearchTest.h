#ifndef MANTID_SINQ_POLDIPEAKSEARCHTEST_H_
#define MANTID_SINQ_POLDIPEAKSEARCHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiPeakSearch.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"

using Mantid::Poldi::PoldiPeakSearch;
using namespace Mantid::Poldi;
using namespace Mantid::Kernel;

class TestablePoldiPeakSearch : public PoldiPeakSearch
{
    friend class PoldiPeakSearchTest;
};

class PoldiPeakSearchTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiPeakSearchTest *createSuite() { return new PoldiPeakSearchTest(); }
    static void destroySuite( PoldiPeakSearchTest *suite ) { delete suite; }


    void testgetNeighborSums()
    {
        double raw[] = {1.0, 2.0, 3.0, 4.0};
        std::vector<double> input(raw, raw + 4);

        TestablePoldiPeakSearch poldiPeakSearch;

        std::vector<double> sum = poldiPeakSearch.getNeighborSums(input);

        TS_ASSERT_EQUALS(sum.size(), 2);
        TS_ASSERT_EQUALS(sum[0], 6.0);
        TS_ASSERT_EQUALS(sum[1], 9.0);

        input.pop_back();
        input.pop_back();

        TS_ASSERT_THROWS(poldiPeakSearch.getNeighborSums(input), std::runtime_error);
    }

    void testSetMinimumDistance()
    {
        TestablePoldiPeakSearch poldiPeakSearch;

        TS_ASSERT_THROWS(poldiPeakSearch.setMinimumDistance(0), std::runtime_error);

        poldiPeakSearch.setMinimumDistance(2);
        TS_ASSERT_EQUALS(poldiPeakSearch.m_minimumDistance, 2);
        TS_ASSERT_EQUALS(poldiPeakSearch.m_doubleMinimumDistance, 4);
    }

    void testsetMaximumPeakNumber()
    {
        TestablePoldiPeakSearch poldiPeakSearch;

        poldiPeakSearch.setMaximumPeakNumber(2);
        TS_ASSERT_EQUALS(poldiPeakSearch.m_maximumPeakNumber, 2);
    }

    void testsetMinimumPeakHeight()
    {
        TestablePoldiPeakSearch poldiPeakSearch;

        poldiPeakSearch.setMinimumPeakHeight(200.0);

        TS_ASSERT_EQUALS(poldiPeakSearch.m_minimumPeakHeight, 200.0);
    }

    void testfindPeaksRecursive()
    {
        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(2);

        double testListRaw[] = {-3.0, -2.0, 12.0, 3.0, 5.0, 7.0, 12.0, 34.0, 13.0, 18.0, 1.0, -10.0, 12.0, 3.0, 4.0, 6.0, 7.0};
        std::vector<double> testList(testListRaw, testListRaw + 17);

        std::list<std::vector<double>::iterator> maxima = poldiPeakSearch.findPeaksRecursive(testList.begin(), testList.end());
        TS_ASSERT_EQUALS(maxima.size(), 3);

        maxima.sort();

        double shouldGiveMaxima[] = {12.0, 34.0, 12.0};

        for(size_t i = 0; i < 3; ++i) {
            TS_ASSERT_EQUALS(*maxima.front(), shouldGiveMaxima[i]);
            maxima.pop_front();
        }

        // Same test with absolute recursion borders gives one additional peak at the right edge
        poldiPeakSearch.setRecursionAbsoluteBorders(testList.begin(), testList.end());
        std::list<std::vector<double>::iterator> edgeCasesMaxima = poldiPeakSearch.findPeaksRecursive(testList.begin(), testList.end());
        TS_ASSERT_EQUALS(edgeCasesMaxima.size(), 4);

        edgeCasesMaxima.sort();

        double shouldGiveAbsoluteBordersMaxima[] = {12.0, 34.0, 12.0, 7.0};

        for(size_t i = 0; i < 4; ++i) {
            TS_ASSERT_EQUALS(*edgeCasesMaxima.front(), shouldGiveAbsoluteBordersMaxima[i]);
            edgeCasesMaxima.pop_front();
        }
    }

    void testsetRecursionAbsoluteBorders()
    {
        TestablePoldiPeakSearch poldiPeakSearch;

        double testListRaw[] = {2.0, -3.0, -2.0, 12.0, 3.0 };
        std::vector<double> baseData(testListRaw, testListRaw + 5);

        poldiPeakSearch.setRecursionAbsoluteBorders(baseData.begin(), baseData.end());

        TS_ASSERT_EQUALS(poldiPeakSearch.m_recursionAbsoluteBegin, baseData.begin());
        TS_ASSERT_EQUALS(poldiPeakSearch.m_recursionAbsoluteEnd, baseData.end());
    }

    void testgetLeftRangeBegin()
    {
        int minimumDistance = 2;

        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(minimumDistance);

        double testListRaw[] = {2.0, -3.0, -2.0, 12.0, 3.0 };
        std::vector<double> baseData(testListRaw, testListRaw + 5);

        TS_ASSERT_EQUALS(poldiPeakSearch.getLeftRangeBegin(baseData.begin()), baseData.begin() + minimumDistance);

        poldiPeakSearch.setRecursionAbsoluteBorders(baseData.begin(), baseData.end());

        TS_ASSERT_EQUALS(poldiPeakSearch.getLeftRangeBegin(baseData.begin()), baseData.begin());
        TS_ASSERT_EQUALS(poldiPeakSearch.getLeftRangeBegin(baseData.end() - minimumDistance), baseData.end());
    }

    void testgetRightRangeEnd()
    {
        int minimumDistance = 2;

        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(minimumDistance);

        double testListRaw[] = {2.0, -3.0, -2.0, 12.0, 3.0 };
        std::vector<double> baseData(testListRaw, testListRaw + 5);

        TS_ASSERT_EQUALS(poldiPeakSearch.getRightRangeEnd(baseData.end()), baseData.end() - minimumDistance);

        poldiPeakSearch.setRecursionAbsoluteBorders(baseData.begin(), baseData.end());

        TS_ASSERT_EQUALS(poldiPeakSearch.getRightRangeEnd(baseData.end()), baseData.end());
        TS_ASSERT_EQUALS(poldiPeakSearch.getRightRangeEnd(baseData.begin() + minimumDistance), baseData.begin());
    }

    void testfindPeaks()
    {
        int maxPeakNum = 2;

        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(2);
        poldiPeakSearch.setMaximumPeakNumber(maxPeakNum);

        double testListRaw[] = {-3.0, -2.0, 12.0, 3.0, 5.0, 7.0, 12.0, 34.0, 13.0, 18.0, 1.0, -10.0, 12.0, 3.0, 4.0, 6.0, 7.0};
        std::vector<double> testList(testListRaw, testListRaw + 17);

        std::list<std::vector<double>::iterator> maxima = poldiPeakSearch.findPeaks(testList.begin(), testList.end());
        TS_ASSERT_EQUALS(maxima.size(), maxPeakNum);
        TS_ASSERT_EQUALS(*maxima.front(), 34.0);        
        TS_ASSERT_EQUALS(*maxima.back(), 12.0);

    }

    void testgetPeakCoordinates()
    {
        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(2);
        poldiPeakSearch.setMaximumPeakNumber(3);

        double testListRaw[] = {2.0, -3.0, -2.0, 12.0, 3.0, 5.0, 7.0, 12.0, 34.0, 13.0, 18.0, 1.0, -10.0, 12.0, 3.0, 4.0, 6.0, 7.0, 3.0};
        std::vector<double> baseData(testListRaw + 1, testListRaw + 17);

        std::vector<double> testYData(testListRaw, testListRaw + 19);
        std::vector<double> testXData(testYData.size());
        double x = 0.0;
        std::generate(testXData.begin(), testXData.end(), [&x]() { return x += 1.0; });

        std::list<std::vector<double>::iterator> maxima = poldiPeakSearch.findPeaksRecursive(baseData.begin(), baseData.end());

        maxima.sort();

        std::vector<PoldiPeak_sptr> peaks = poldiPeakSearch.getPeaks(baseData.begin(), maxima, testXData);

        TS_ASSERT_EQUALS(peaks.size(), 3);

        PoldiPeak_sptr peak0 = peaks[0];
        TS_ASSERT_EQUALS(peak0->q(), 4.0);
        TS_ASSERT_EQUALS(peak0->intensity(), 12.0 / 3.0);

        PoldiPeak_sptr peak1 = peaks[1];
        TS_ASSERT_EQUALS(peak1->q(), 9.0);
        TS_ASSERT_EQUALS(peak1->intensity(), 34.0 / 3.0);
    }

    void testmapPeakPositionsToCorrelationData()
    {
        TestablePoldiPeakSearch poldiPeakSearch;

        std::vector<double> firstVector;
        firstVector.push_back(2.0);
        firstVector.push_back(3.0);
        firstVector.push_back(4.0);
        firstVector.push_back(5.0);

        std::vector<double> secondVector;
        secondVector.push_back(1.5);
        secondVector.push_back(2.5);
        secondVector.push_back(3.5);
        secondVector.push_back(4.5);
        secondVector.push_back(5.5);
        secondVector.push_back(6.5);

        std::list<std::vector<double>::iterator> firstIterators;
        firstIterators.push_back(firstVector.begin() + 2);
        firstIterators.push_back(firstVector.begin() + 3);

        std::list<std::vector<double>::iterator> secondIterators = poldiPeakSearch.mapPeakPositionsToCorrelationData(firstIterators, firstVector.begin(), secondVector.begin());

        TS_ASSERT_EQUALS(*secondIterators.front(), 4.5);
        TS_ASSERT_EQUALS(*secondIterators.back(), 5.5);
    }

    void testgetNumberOfBackgroundPoints()
    {
        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(2);

        std::list<std::vector<double>::iterator> peakPositions(4);
        std::vector<double> correlationCounts(30);

        TS_ASSERT_EQUALS(poldiPeakSearch.getNumberOfBackgroundPoints(peakPositions, correlationCounts), 8);

        std::list<std::vector<double>::iterator> tooManyPeaks(40);
        TS_ASSERT_THROWS(poldiPeakSearch.getNumberOfBackgroundPoints(tooManyPeaks, correlationCounts), std::runtime_error);
    }

    void testgetBackgroundWithSigma()
    {
        TestablePoldiPeakSearch poldiPeakSearch;
        poldiPeakSearch.setMinimumDistance(2);

        double rawTestList[] = { 1.0, 2.0, 1.0, 2.0, 1.0, 0.0, 4.0, 0.0, 1.0, 2.0, 1.0, 2.0, 1.0};
        std::vector<double> testList(rawTestList, rawTestList + 13);

        std::list<std::vector<double>::iterator> peaks;
        peaks.push_front(testList.begin() + 6);

        TS_ASSERT_EQUALS(poldiPeakSearch.getNumberOfBackgroundPoints(peaks, testList), 6);

        UncertainValue bgSigma = poldiPeakSearch.getBackgroundWithSigma(peaks, testList);
        TS_ASSERT_EQUALS(bgSigma.value(), 10.0/6.0);
        TS_ASSERT_EQUALS(bgSigma.error(), 1.0);
    }

    void testminimumPeakHeightFromBackground()
    {
        TestablePoldiPeakSearch poldiPeakSearch;

        TS_ASSERT_EQUALS(poldiPeakSearch.minimumPeakHeightFromBackground(UncertainValue(3.0, 3.5)), 12.625);
    }
};


#endif /* MANTID_SINQ_POLDIPEAKSEARCHTEST_H_ */
