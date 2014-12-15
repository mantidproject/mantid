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
        // Replace static member variables for property names by class members
        m_testableChopperSpeedPropertyName = "ChopperSpeed";
        m_testableChopperSpeedTargetPropertyName = "ChopperSpeedTarget";

        // special properties for testing AbstractDoubleValueExtractor
        m_run.addProperty<double>("chopperspeed_double", 10000.0);

        std::vector<double> chopperSpeed(1, 10000.0);
        m_run.addProperty<std::vector<double> >("chopperspeed_vector", chopperSpeed);

        std::vector<int> chopperSpeedTargetsInt(1, 10000);
        m_run.addProperty<std::vector<int> >("chopperspeed_target_int_vector", chopperSpeedTargetsInt);

        // add string property, for which there is no extractor
        m_stringRun.addProperty<std::string>(getChopperSpeedPropertyName(), "10000.0");
        m_stringRun.addProperty<std::string>(getChopperSpeedTargetPropertyName(), "10000.0");


        // run with correct chopperspeed property
        m_correctRun.addProperty<double>(getChopperSpeedPropertyName(), 10000.0);
        m_correctRun.addProperty<double>(getChopperSpeedTargetPropertyName(), 10000.0);
    }

    void testVectorDoubleValueExtractor()
    {
        // Extract vector value with vector value extractor - this should work.
        AbstractDoubleValueExtractor_sptr extractorGood(new VectorDoubleValueExtractor);
        TS_ASSERT_THROWS_NOTHING((*extractorGood)(const_cast<const Run &>(m_run), "chopperspeed_vector"));

        // this should not work, because it's a "number" property (see constructor above)
        AbstractDoubleValueExtractor_sptr extractorBad(new VectorDoubleValueExtractor);
        TS_ASSERT_THROWS((*extractorBad)(const_cast<const Run &>(m_run), "chopperspeed_double"), std::invalid_argument);

        // check that the value comes out correctly
        TS_ASSERT_EQUALS((*extractorGood)(const_cast<const Run &>(m_run), "chopperspeed_vector"), 10000.0);
    }

    void testVectorIntValueExtractor()
    {
        // Extract vector value with vector value extractor - this should work.
        AbstractDoubleValueExtractor_sptr extractorGood(new VectorIntValueExtractor);
        TS_ASSERT_THROWS_NOTHING((*extractorGood)(const_cast<const Run &>(m_run), "chopperspeed_target_int_vector"));

        // this should not work, because it's a "number" property (see constructor above)
        AbstractDoubleValueExtractor_sptr extractorBad(new VectorDoubleValueExtractor);
        TS_ASSERT_THROWS((*extractorBad)(const_cast<const Run &>(m_run), "chopperspeed_double"), std::invalid_argument);

        // check that the value comes out correctly
        TS_ASSERT_EQUALS((*extractorGood)(const_cast<const Run &>(m_run), "chopperspeed_target_int_vector"), 10000.0);
    }

    void testNumberDoubleValueExtractor()
    {
        // Same as above test
        AbstractDoubleValueExtractor_sptr extractorGood(new NumberDoubleValueExtractor);
        TS_ASSERT_THROWS_NOTHING((*extractorGood)(const_cast<const Run &>(m_run), "chopperspeed_double"));

        AbstractDoubleValueExtractor_sptr extractorBad(new NumberDoubleValueExtractor);
        TS_ASSERT_THROWS((*extractorBad)(const_cast<const Run &>(m_run), "chopperspeed_vector"), std::invalid_argument);

        // check that the value comes out correctly
        TS_ASSERT_EQUALS((*extractorGood)(const_cast<const Run &>(m_run), "chopperspeed_double"), 10000.0);
    }

    void testGetChopperSpeedFromRun() {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        TS_ASSERT_EQUALS(instrumentAdapter.getChopperSpeedFromRun(m_correctRun), 10000.0);
    }

    void testGetChopperSpeedTargetFromRun()
    {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        TS_ASSERT_EQUALS(instrumentAdapter.getChopperSpeedTargetFromRun(m_correctRun), 10000.0);
    }

    void testExtractPropertyFromRun() {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        // Throws, because "chopperspeed" is missing
        TS_ASSERT_THROWS(instrumentAdapter.extractPropertyFromRun(m_run, "DOESNOTEXIST"), std::runtime_error);

        // Throws, because there is no extractor for supplied type
        const std::string propertyName = getChopperSpeedPropertyName();
        TS_ASSERT_THROWS(instrumentAdapter.extractPropertyFromRun(m_stringRun, propertyName), std::invalid_argument);

        // Should be ok.
        TS_ASSERT_THROWS_NOTHING(instrumentAdapter.extractPropertyFromRun(m_correctRun, propertyName));
        TS_ASSERT_EQUALS(instrumentAdapter.extractPropertyFromRun(m_correctRun, propertyName), 10000.0);
    }

    void testChopperSpeedMatchesTarget() {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        // This throws, because there is no extractor and the exception is not caught in the method
        TS_ASSERT_THROWS(instrumentAdapter.chopperSpeedMatchesTarget(m_stringRun, 10000.0), std::invalid_argument);

        // If the property is not present, it is an old file and there can't be any comparison, so it's always true
        TS_ASSERT_THROWS_NOTHING(instrumentAdapter.chopperSpeedMatchesTarget(m_run, 10000.0));
        TS_ASSERT_EQUALS(instrumentAdapter.chopperSpeedMatchesTarget(m_run, 10000.0), true);
        TS_ASSERT_EQUALS(instrumentAdapter.chopperSpeedMatchesTarget(m_run, 100.0), true);

        // Otherwise, the values are compared with a tolerance of 1e-4
        TS_ASSERT_EQUALS(instrumentAdapter.chopperSpeedMatchesTarget(m_correctRun, 10000.0), true);
        TS_ASSERT_EQUALS(instrumentAdapter.chopperSpeedMatchesTarget(m_correctRun, 10000.00009), true);
        TS_ASSERT_EQUALS(instrumentAdapter.chopperSpeedMatchesTarget(m_correctRun, 10000.0002), false);
        TS_ASSERT_EQUALS(instrumentAdapter.chopperSpeedMatchesTarget(m_correctRun, 9000.0), false);
    }

    void testGetCleanChopperSpeed() {
        TestablePoldiInstrumentAdapter instrumentAdapter;

        TS_ASSERT_EQUALS(instrumentAdapter.getCleanChopperSpeed(4750.0), 5000.0);
        TS_ASSERT_EQUALS(instrumentAdapter.getCleanChopperSpeed(4749.9), 4500.0);
        TS_ASSERT_EQUALS(instrumentAdapter.getCleanChopperSpeed(4999.3), 5000.0);
        TS_ASSERT_EQUALS(instrumentAdapter.getCleanChopperSpeed(5001.0), 5000.0);
        TS_ASSERT_EQUALS(instrumentAdapter.getCleanChopperSpeed(12499.1), 12500.0);
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
        extractor = instrumentAdapter.getExtractorForProperty(m_stringRun.getProperty(getChopperSpeedPropertyName()));
        TS_ASSERT(!extractor);
    }

private:
    boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
    boost::shared_ptr<MockChopper> m_chopper;
    PoldiSourceSpectrum_sptr m_spectrum;

    Run m_run;
    Run m_correctRun;
    Run m_stringRun;

    std::string m_testableChopperSpeedPropertyName;
    std::string m_testableChopperSpeedTargetPropertyName;


    std::string getChopperSpeedPropertyName()
    {
        return m_testableChopperSpeedPropertyName;
    }

    std::string getChopperSpeedTargetPropertyName()
    {
        return m_testableChopperSpeedTargetPropertyName;
    }

    class TestablePoldiInstrumentAdapter : public PoldiInstrumentAdapter
    {
        friend class PoldiInstrumentAdapterTest;
    public:
        TestablePoldiInstrumentAdapter() : PoldiInstrumentAdapter() { }
        ~TestablePoldiInstrumentAdapter() { }
    };
};

#endif // MANTID_SINQ_POLDIINSTRUMENTADAPTERTEST_H_
