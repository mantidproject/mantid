#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/FitMW.h"

using ::testing::Return;

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

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

        m_instrument = PoldiInstrumentAdapter_sptr(new FakePoldiInstrumentAdapter);
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

    void testChopperSlitOffsets()
    {
        TestablePoldiSpectrumDomainFunction function;

        std::vector<double> offsets = function.getChopperSlitOffsets(m_chopper);

        for(size_t i = 0; i < offsets.size(); ++i) {
            TS_ASSERT_EQUALS(offsets[i], m_chopper->slitTimes()[i] + m_chopper->zeroOffset());
        }
    }

    void testInitializeFromInstrument()
    {
        TestablePoldiSpectrumDomainFunction function;
        function.initializeInstrumentParameters(m_instrument);

        TS_ASSERT_EQUALS(function.m_chopperSlitOffsets.size(), m_chopper->slitPositions().size());
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
        function.setParameter("Centre", 1.1086444);

        function.m_deltaT = 3.0;
        function.initializeInstrumentParameters(m_instrument);


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

    void testAccessThroughBasePointer()
    {
        TestablePoldiSpectrumDomainFunction *function = new TestablePoldiSpectrumDomainFunction();
        function->initialize();
        function->setParameter("Area", 1.9854805);
        function->setParameter("Fwhm", 0.0027446316797104233);
        function->setParameter("Centre", 1.1086444);

        function->m_deltaT = 3.0;
        function->initializeInstrumentParameters(m_instrument);

        TS_ASSERT_EQUALS(function->getParameter(2), 1.1086444);

        MultiDomainFunction *mdf = new MultiDomainFunction();
        mdf->addFunction(IFunction_sptr(dynamic_cast<IFunction *>(function)));

        TS_ASSERT_EQUALS(static_cast<IFunction*>(mdf)->getParameter(2), 1.1086444);
    }

   /*
    * This test must be re-enabled, when #9497 is fixed, then it will pass.
    *
    void ___testCreateInitialized()
    {
        IFunction_sptr function(new Gaussian());
        function->initialize();
        function->setParameter(0, 1.23456);
        function->setParameter(1, 1.234567);
        function->setParameter(2, 0.01234567);

        IFunction_sptr clone = function->clone();

        // passes, Parameter 0 has less than 7 significant digits
        TS_ASSERT_EQUALS(function->getParameter(0), clone->getParameter(0));

        // fails, Parameter 1 has more than 7 significant digits
        TS_ASSERT_EQUALS(function->getParameter(1), clone->getParameter(1));

        // fails, Parameter 2 has more than 7 significant digits
        TS_ASSERT_EQUALS(function->getParameter(2), clone->getParameter(2));
    }
    */

private:
    class TestablePoldiSpectrumDomainFunction : PoldiSpectrumDomainFunction
    {
        friend class PoldiSpectrumDomainFunctionTest;

        TestablePoldiSpectrumDomainFunction() : PoldiSpectrumDomainFunction() {}
    };

    boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
    boost::shared_ptr<MockChopper> m_chopper;
    PoldiSourceSpectrum_sptr m_spectrum;

    PoldiInstrumentAdapter_sptr m_instrument;
};


#endif /* MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTIONTEST_H_ */
