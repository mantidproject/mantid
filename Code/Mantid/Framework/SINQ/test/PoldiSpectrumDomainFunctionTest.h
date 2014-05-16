#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using ::testing::Return;

using namespace Mantid::Poldi;

class PoldiSpectrumDomainFunctionTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiSpectrumDomainFunctionTest *createSuite() { return new PoldiSpectrumDomainFunctionTest(); }
    static void destroySuite( PoldiSpectrumDomainFunctionTest *suite ) { delete suite; }

    PoldiSpectrumDomainFunctionTest()
    {
        m_detector = boost::shared_ptr<ConfiguredHeliumDetector>(new ConfiguredHeliumDetector);
        m_chopper = boost::shared_ptr<MockChopper>(new MockChopper);

        m_spectrum = PoldiSourceSpectrum_sptr(new ConfiguredSpectrum);

        EXPECT_CALL(*m_chopper, distanceFromSample())
                .WillRepeatedly(Return(11800.0));

        EXPECT_CALL(*m_chopper, zeroOffset())
                .WillRepeatedly(Return(0.15));
    }


    void testInit()
    {
        PoldiSpectrumDomainFunction function;
        function.initialize();

        std::vector<std::string> parameterNames = function.getParameterNames();

        TS_ASSERT_EQUALS(parameterNames[0], "Area");
        TS_ASSERT_EQUALS(parameterNames[1], "Fwhm");
        TS_ASSERT_EQUALS(parameterNames[2], "Centre");
    }

    void testDetectorCharacteristics()
    {
        double distance = 1996.017;
        double tof1A = 4947.990234375;
        double twoTheta = 1.577358;

        double sinTheta = 0.70942287322834615878;
        double cosTheta = 0.70478307793280472246;

        DetectorElementCharacteristics characteristics(static_cast<int>(m_detector->centralElement()), m_detector, m_chopper);

        TS_ASSERT_DELTA(characteristics.twoTheta, twoTheta, 1e-6);
        TS_ASSERT_DELTA(characteristics.distance, distance, 1e-3);
        TS_ASSERT_DELTA(characteristics.totalDistance, distance + 11800.0, 1e-3);
        TS_ASSERT_DELTA(characteristics.tof1A, tof1A, 1e-4);
        TS_ASSERT_DELTA(characteristics.sinTheta, sinTheta, 1e-6);
        TS_ASSERT_DELTA(characteristics.cosTheta, cosTheta, 1e-6);

        TestablePoldiSpectrumDomainFunction function;
        DetectorElementCharacteristics center = function.getDetectorCenterCharacteristics(m_detector, m_chopper);

        TS_ASSERT_EQUALS(characteristics.twoTheta, center.twoTheta);
        TS_ASSERT_EQUALS(characteristics.distance, center.distance);
        TS_ASSERT_EQUALS(characteristics.totalDistance, center.totalDistance);
        TS_ASSERT_EQUALS(characteristics.tof1A, center.tof1A);
        TS_ASSERT_EQUALS(characteristics.sinTheta, center.sinTheta);
        TS_ASSERT_EQUALS(characteristics.cosTheta, center.cosTheta);
    }

    void testDetectorFactors()
    {
        DetectorElementCharacteristics center(static_cast<int>(m_detector->centralElement()), m_detector, m_chopper);

        DetectorElementData data(102, center, m_detector, m_chopper);

        TS_ASSERT_DELTA(data.intensityFactor(), 1.010685, 1e-6);
        //TS_ASSERT_DELTA(data.lambdaFactor(), 2.6941614e-4, 1e-11);
        TS_ASSERT_DELTA(data.timeFactor(), 0.9346730, 1e-7);
    }

    void testChopperSlitOffsets()
    {
        TestablePoldiSpectrumDomainFunction function;

        std::vector<double> offsets = function.getChopperSlitOffsets(m_chopper);

        for(size_t i = 0; i < offsets.size(); ++i) {
            TS_ASSERT_EQUALS(offsets[i], m_chopper->slitTimes()[i] + m_chopper->zeroOffset());
        }
    }

    void testGetDetectorElementData()
    {
        TestablePoldiSpectrumDomainFunction function;
        std::vector<DetectorElementData_const_sptr> elements = function.getDetectorElementData(m_detector, m_chopper);
        DetectorElementCharacteristics center = function.getDetectorCenterCharacteristics(m_detector, m_chopper);

        DetectorElementData data(102, center, m_detector, m_chopper);

        TS_ASSERT_EQUALS(data.intensityFactor(), elements[102]->intensityFactor());
        //TS_ASSERT_DELTA(data.lambdaFactor(), 2.6941614e-4, 1e-11);
        //TS_ASSERT_DELTA(data.timeFactor(), 0.9346730, 1e-7);
    }

    void testInitializeFromInstrument()
    {
        TestablePoldiSpectrumDomainFunction function;
        function.initializeFromInstrument(m_detector, m_chopper);

        TS_ASSERT_EQUALS(function.m_chopperSlitOffsets.size(), m_chopper->slitPositions().size());
        TS_ASSERT_EQUALS(function.m_detectorCenter.twoTheta, m_detector->twoTheta(static_cast<int>(m_detector->centralElement())));
        TS_ASSERT_EQUALS(function.m_detectorElementData.size(), m_detector->elementCount());
    }

    void testTimeTransformedWidth()
    {
        /* Values from existing analysis software */
        double fwhm = 0.0027446316797104233;
        double deltaT = 3.0;

        TestablePoldiSpectrumDomainFunction function;
        function.initializeFromInstrument(m_detector, m_chopper);
        //double fwhmT = function.dToTOF(fwhm);
        double fwhmT = fwhm * 4947.990;

        TS_ASSERT_DELTA(function.timeTransformedWidth(fwhmT, 342) / deltaT, 4.526804, 1e-5);
    }

    void testTimeTransformedCentre()
    {
        double centre = 1.10864434901480127601;

        TestablePoldiSpectrumDomainFunction function;
        function.initializeFromInstrument(m_detector, m_chopper);
        double centreT = function.dToTOF(centre);
        //double centreT = centre * 4947.990;

        TS_ASSERT_DELTA(function.timeTransformedCentre(centreT, 342), 5964.820800781, 1e-3);
    }

    void testTimeTransformedIntensity()
    {
        double centre = 1.10864434901480127601;
        double areaD = 1.985481;

        TestablePoldiSpectrumDomainFunction function;
        function.m_spectrum = m_spectrum;
        function.initializeFromInstrument(m_detector, m_chopper);
        function.m_detectorEfficiency = 0.88;
        double centreT = function.dToTOF(centre);
        //double centreT = centre * 4947.990;
        TS_ASSERT_DELTA(function.timeTransformedIntensity(areaD, centreT, 342), 4.611182, 1e-5);
    }

    void testActualFunction()
    {
        /* comparison with results from a math program */
        double area = 1.0;
        double sigma = 1.0;
        double x0 = 0.0;

        std::vector<double> reference;
        reference.push_back(0.388349126567583);
        reference.push_back(0.398942280401433);
        reference.push_back(0.359646701831886);
        reference.push_back(0.004431848411938);

        TestablePoldiSpectrumDomainFunction function;

        std::vector<double> x;
        x.push_back(-0.232);
        x.push_back(0.0);
        x.push_back(0.4554);
        x.push_back(3.0);

        for(size_t i = 0; i < x.size(); ++i) {
            TS_ASSERT_DELTA(function.actualFunction(x[i], x0, sigma, area), reference[i], 1e-15);
        }
    }

    void testFunction()
    {
        TestablePoldiSpectrumDomainFunction function;
        function.initialize();
        function.setParameter("Area", 1.9854805);
        function.setParameter("Fwhm", 0.0027446316797104233);
        //function.setParameter("Centre", 1.10864434901480127601);
        function.setParameter("Centre", 1.1086444);//(2.0 * M_PI) / 5.667449);//1.10864434901480127601);

        function.initializeFromInstrument(m_detector, m_chopper);
        function.m_deltaT = 3.0;
        function.m_spectrum = m_spectrum;

        std::vector<double> xvalues(500, 1.0);

        FunctionDomain1DSpectrum domain(342, xvalues);
        TS_ASSERT_EQUALS(domain.getWorkspaceIndex(), 342);
        FunctionValues values(domain);
        values.setCalculated(0.0);

        function.function(domain, values);

        std::vector<double> reference;
        reference.push_back(2.5469337E-05);
        reference.push_back(2.4222479E-04);
        reference.push_back(1.7575109E-03);
        reference.push_back(9.7287362E-03);
        reference.push_back(4.1085955E-02);
        reference.push_back(0.1323760);
        reference.push_back(0.3253897);
        reference.push_back(0.6102068);
        reference.push_back(0.8730301);
        reference.push_back(0.9529279);
        reference.push_back(0.7935416);
        reference.push_back(0.5041480);
        reference.push_back(0.2443572);
        reference.push_back(9.0358935E-02);
        reference.push_back(2.5491528E-02);
        reference.push_back(5.4865498E-03);
        reference.push_back(9.0091029E-04);
        reference.push_back(1.1286059E-04);
        reference.push_back(1.0786535E-05);

        for(size_t i = 0; i < reference.size(); ++i) {
            TS_ASSERT_DELTA(values[479 + i], reference[i], 9e-5);
        }
    }

private:
    class TestablePoldiSpectrumDomainFunction : PoldiSpectrumDomainFunction
    {
        friend class PoldiSpectrumDomainFunctionTest;

        TestablePoldiSpectrumDomainFunction() : PoldiSpectrumDomainFunction() {}
    };

    boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
    boost::shared_ptr<MockChopper> m_chopper;
    PoldiSourceSpectrum_sptr m_spectrum;
};


#endif /* MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_ */
