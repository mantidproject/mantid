#ifndef MANTID_PARALLEL_PULSETIMEGENERATORTEST_H_
#define MANTID_PARALLEL_PULSETIMEGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include <type_traits>

#include "MantidParallel/IO/PulseTimeGenerator.h"

using PulseTimeGenerator =
    Mantid::Parallel::IO::PulseTimeGenerator<int32_t, int32_t>;
using Mantid::Parallel::IO::detail::IntOrFloat64Bit;
using Mantid::Parallel::IO::detail::scaleFromUnit;

class PulseTimeGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PulseTimeGeneratorTest *createSuite() {
    return new PulseTimeGeneratorTest();
  }
  static void destroySuite(PulseTimeGeneratorTest *suite) { delete suite; }

  void test_scaleFromUnit_integer_converted_to_nanoseconds() {
    // DateAndTime expects int64_t to be in nanoseconds so if unit does not
    // match there must be an appropriate conversion factor.
    TS_ASSERT_EQUALS(scaleFromUnit<int32_t>("nanosecond"), 1);
    TS_ASSERT_EQUALS(scaleFromUnit<uint32_t>("nanosecond"), 1);
    TS_ASSERT_EQUALS(scaleFromUnit<int64_t>("nanosecond"), 1);
    TS_ASSERT_EQUALS(scaleFromUnit<uint64_t>("nanosecond"), 1);
    // Would supporting anything by `second` make sense for integers?
    TS_ASSERT_THROWS_EQUALS(
        scaleFromUnit<int64_t>("second"), const std::runtime_error &e,
        std::string(e.what()),
        "PulseTimeGenerator: unsupported unit `second` for event_time_zero");
  }

  void test_scaleFromUnit_float_converted_to_microseconds() {
    // DateAndTime expects double to be in seconds so if unit does not match
    // there must be an appropriate conversion factor.
    TS_ASSERT_EQUALS(scaleFromUnit<float>("second"), 1.0);
    TS_ASSERT_EQUALS(scaleFromUnit<double>("second"), 1.0);
    TS_ASSERT_EQUALS(scaleFromUnit<float>("microsecond"), 1e-6);
    TS_ASSERT_EQUALS(scaleFromUnit<double>("microsecond"), 1e-6);
    TS_ASSERT_EQUALS(scaleFromUnit<float>("nanosecond"), 1e-9);
    TS_ASSERT_EQUALS(scaleFromUnit<double>("nanosecond"), 1e-9);
    // Currently not supported (but in principle we could).
    TS_ASSERT_THROWS_EQUALS(scaleFromUnit<float>("millisecond"),
                            const std::runtime_error &e, std::string(e.what()),
                            "PulseTimeGenerator: unsupported unit "
                            "`millisecond` for event_time_zero");
  }

  void test_scaleFromUnit_does_not_lose_precision() {
    // Return type should be double, even if input is float
    TS_ASSERT_DIFFERS(scaleFromUnit<float>("nanosecond"),
                      static_cast<float>(1e-9));
  }

  void test_IntOrFloat64Bit() {
    TS_ASSERT((std::is_same<IntOrFloat64Bit<int32_t>::type, int64_t>::value));
    TS_ASSERT((std::is_same<IntOrFloat64Bit<uint32_t>::type, int64_t>::value));
    TS_ASSERT((std::is_same<IntOrFloat64Bit<int64_t>::type, int64_t>::value));
    TS_ASSERT((std::is_same<IntOrFloat64Bit<uint64_t>::type, int64_t>::value));
    TS_ASSERT((std::is_same<IntOrFloat64Bit<float>::type, double>::value));
    TS_ASSERT((std::is_same<IntOrFloat64Bit<double>::type, double>::value));
  }

  void test_empty() {
    PulseTimeGenerator pulseTimes({}, {}, "nanosecond", 1000);
    TS_ASSERT_THROWS_EQUALS(pulseTimes.seek(0), const std::runtime_error &e,
                            std::string(e.what()),
                            "Empty event index in PulseTimeGenerator");
  }

  void test_no_seek() {
    PulseTimeGenerator pulseTimes({0}, {17}, "nanosecond", 1000);
    // seek() must always called before the first next() call
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 0);
  }

  void test_size_1() {
    PulseTimeGenerator pulseTimes({0}, {17}, "nanosecond", 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1017);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1017);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1017);
  }

  void test_size_2() {
    PulseTimeGenerator pulseTimes({0, 2}, {4, 8}, "nanosecond", 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
  }

  void test_empty_pulse_at_start() {
    PulseTimeGenerator pulseTimes({0, 0, 2}, {4, 8, 12}, "nanosecond", 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
  }

  void test_empty_pulse() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, "nanosecond",
                                  1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
  }

  void test_empty_pulse_at_end() {
    PulseTimeGenerator pulseTimes({0, 2, 2}, {4, 8, 12}, "nanosecond", 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
  }

  void test_seek_to_pulse() {
    PulseTimeGenerator pulseTimes({0, 2}, {4, 8}, "nanosecond", 1000);
    pulseTimes.seek(2);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
  }

  void test_seek_into_pulse() {
    PulseTimeGenerator pulseTimes({0, 2}, {4, 8}, "nanosecond", 1000);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
  }

  void test_seek_with_empty_pulse() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, "nanosecond",
                                  1000);
    pulseTimes.seek(2);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
  }

  void test_seek_multiple_times() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, "nanosecond",
                                  1000);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    pulseTimes.seek(3);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
  }

  void test_seek_backwards() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, "nanosecond",
                                  1000);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
  }

  void test_event_time_zero_type_conversion() {
    Mantid::Parallel::IO::PulseTimeGenerator<int32_t, float> pulseTimes(
        {0}, {1.5}, "microsecond", 10000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 11500);
  }
};

#endif /* MANTID_PARALLEL_PULSETIMEGENERATORTEST_H_ */
