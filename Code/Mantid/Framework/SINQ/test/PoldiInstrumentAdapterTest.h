#ifndef MANTID_SINQ_POLDIINSTRUMENTADAPTERTEST_H_
#define MANTID_SINQ_POLDIINSTRUMENTADAPTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"

#include "MantidAPI/Run.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class PoldiInstrumentAdapterTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiInstrumentAdapterTest *createSuite() { return new PoldiInstrumentAdapterTest(); }
    static void destroySuite( PoldiInstrumentAdapterTest *suite ) { delete suite; }

    PoldiInstrumentAdapterTest()
    {
        // special properties for testing AbstractDoubleValueExtractor
        m_run.addProperty<double>("chopperspeed_double", 10000.0);

        std::vector<double> chopperSpeed(1, 10000.0);
        m_run.addProperty<std::vector<double> >("chopperspeed_vector", chopperSpeed);

        // add string property, for which there is no extractor
        m_stringRun.addProperty<std::string>(PoldiInstrumentAdapter::getChopperSpeedPropertyName(), "10000.0");

        // run with correct chopperspeed property
        m_correctRun.addProperty<double>(PoldiInstrumentAdapter::getChopperSpeedPropertyName(), 10000.0);
    }

    void testVectorDoubleValueExtractor()
    {
        // Extract vector value with vector value extractor - this should work.
        AbstractDoubleValueExtractor_sptr extractorGood(new VectorDoubleValueExtractor("chopperspeed_vector"));
        TS_ASSERT_THROWS_NOTHING((*extractorGood)(const_cast<const Run &>(m_run)));

        // this should not work, because it's a "number" property (see constructor above)
        AbstractDoubleValueExtractor_sptr extractorBad(new VectorDoubleValueExtractor("chopperspeed_double"));
        TS_ASSERT_THROWS((*extractorBad)(const_cast<const Run &>(m_run)), std::invalid_argument);

        // check that the value comes out correctly
        TS_ASSERT_EQUALS((*extractorGood)(const_cast<const Run &>(m_run)), 10000.0);
    }

    void testNumberDoubleValueExtractor()
    {
        // Same as above test
        AbstractDoubleValueExtractor_sptr extractorGood(new NumberDoubleValueExtractor("chopperspeed_double"));
        TS_ASSERT_THROWS_NOTHING((*extractorGood)(const_cast<const Run &>(m_run)));

        AbstractDoubleValueExtractor_sptr extractorBad(new NumberDoubleValueExtractor("chopperspeed_vector"));
        TS_ASSERT_THROWS((*extractorBad)(const_cast<const Run &>(m_run)), std::invalid_argument);

        // check that the value comes out correctly
        TS_ASSERT_EQUALS((*extractorGood)(const_cast<const Run &>(m_run)), 10000.0);
    }

    void testGetChopperSpeedFromRun() {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        // Throws, because "chopperspeed" is missing
        TS_ASSERT_THROWS(instrumentAdapter.getChopperSpeedFromRun(m_run), std::runtime_error);

        // Throws, because there is no extractor for supplied type
        TS_ASSERT_THROWS(instrumentAdapter.getChopperSpeedFromRun(m_stringRun), std::invalid_argument);

        // Should be ok.
        TS_ASSERT_THROWS_NOTHING(instrumentAdapter.getChopperSpeedFromRun(m_correctRun));
        TS_ASSERT_EQUALS(instrumentAdapter.getChopperSpeedFromRun(m_correctRun), 10000.0);
    }

    void testGetExtractorForProperty() {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        // Throw on null-pointer
        TS_ASSERT_THROWS(instrumentAdapter.getExtractorForProperty(0), std::invalid_argument);
        TS_ASSERT_THROWS_NOTHING(instrumentAdapter.getExtractorForProperty(m_run.getProperty("chopperspeed_double")));

        // Check that the correct extractor is retrieved
        AbstractDoubleValueExtractor_sptr extractor = instrumentAdapter.getExtractorForProperty(m_run.getProperty("chopperspeed_double"));
        TS_ASSERT(extractor);

        TS_ASSERT(boost::dynamic_pointer_cast<NumberDoubleValueExtractor>(extractor));
        TS_ASSERT(!boost::dynamic_pointer_cast<VectorDoubleValueExtractor>(extractor));

        // Check that the correct extractor is retrieved
        extractor = instrumentAdapter.getExtractorForProperty(m_run.getProperty("chopperspeed_vector"));
        TS_ASSERT(extractor);

        TS_ASSERT(boost::dynamic_pointer_cast<VectorDoubleValueExtractor>(extractor));
        TS_ASSERT(!boost::dynamic_pointer_cast<NumberDoubleValueExtractor>(extractor));

        // unregistered property type - invalid extractor
        extractor = instrumentAdapter.getExtractorForProperty(m_stringRun.getProperty(PoldiInstrumentAdapter::getChopperSpeedPropertyName()));
        TS_ASSERT(!extractor);
    }

private:
    boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
    boost::shared_ptr<MockChopper> m_chopper;
    PoldiSourceSpectrum_sptr m_spectrum;

    Run m_run;
    Run m_correctRun;
    Run m_stringRun;

    class TestablePoldiInstrumentAdapter : public PoldiInstrumentAdapter
    {
        friend class PoldiInstrumentAdapterTest;
    public:
        TestablePoldiInstrumentAdapter() : PoldiInstrumentAdapter() { }
        ~TestablePoldiInstrumentAdapter() { }
    };
};

#endif // MANTID_SINQ_POLDIINSTRUMENTADAPTERTEST_H_
