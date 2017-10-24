#ifndef MANTID_PARALLEL_PULSETIMEGENERATORTEST_H_
#define MANTID_PARALLEL_PULSETIMEGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/PulseTimeGenerator.h"

using PulseTimeGenerator =
    Mantid::Parallel::IO::PulseTimeGenerator<int32_t, int32_t>;

class PulseTimeGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PulseTimeGeneratorTest *createSuite() {
    return new PulseTimeGeneratorTest();
  }
  static void destroySuite(PulseTimeGeneratorTest *suite) { delete suite; }

  void test_empty() {
    PulseTimeGenerator pulseTimes({}, {}, 1000);
    TS_ASSERT_THROWS_EQUALS(pulseTimes.seek(0), const std::runtime_error &e,
                            std::string(e.what()),
                            "Empty event index in PulseTimeGenerator");
  }

  void test_no_seek() {
    PulseTimeGenerator pulseTimes({0}, {17}, 1000);
    // seek() must always called before the first next() call
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 0);
  }

  void test_size_1() {
    PulseTimeGenerator pulseTimes({0}, {17}, 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1017);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1017);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1017);
  }

  void test_size_2() {
    PulseTimeGenerator pulseTimes({0, 2}, {4, 8}, 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
  }

  void test_empty_pulse_at_start() {
    PulseTimeGenerator pulseTimes({0, 0, 2}, {4, 8, 12}, 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
  }

  void test_empty_pulse() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
  }

  void test_empty_pulse_at_end() {
    PulseTimeGenerator pulseTimes({0, 2, 2}, {4, 8, 12}, 1000);
    pulseTimes.seek(0);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
  }

  void test_seek_to_pulse() {
    PulseTimeGenerator pulseTimes({0, 2}, {4, 8}, 1000);
    pulseTimes.seek(2);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
  }

  void test_seek_into_pulse() {
    PulseTimeGenerator pulseTimes({0, 2}, {4, 8}, 1000);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1008);
  }

  void test_seek_with_empty_pulse() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, 1000);
    pulseTimes.seek(2);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
  }

  void test_seek_multiple_times() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, 1000);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    pulseTimes.seek(3);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1016);
  }

  void test_seek_backwards() {
    PulseTimeGenerator pulseTimes({0, 2, 2, 3}, {4, 8, 12, 16}, 1000);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
    pulseTimes.seek(1);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1004);
    TS_ASSERT_EQUALS(pulseTimes.next().totalNanoseconds(), 1012);
  }
};

#endif /* MANTID_PARALLEL_PULSETIMEGENERATORTEST_H_ */
