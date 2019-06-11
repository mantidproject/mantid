// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_EVENTLOADERTEST_H_
#define MANTID_PARALLEL_EVENTLOADERTEST_H_

#include "MantidTestHelpers/ParallelRunner.h"
#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/EventLoader.h"
#include "MantidParallel/IO/EventParser.h"
#include "MantidParallel/IO/NXEventDataSource.h"
#include "MantidTypes/Event/TofEvent.h"

#include <H5Cpp.h>

namespace Mantid {
namespace Parallel {
namespace IO {
namespace EventLoader {
template <class T> void load() { throw std::runtime_error("unknown"); }
template <> void load<int32_t>() { throw std::runtime_error("int32_t"); }
template <> void load<int64_t>() { throw std::runtime_error("int64_t"); }
template <> void load<uint32_t>() { throw std::runtime_error("uint32_t"); }
template <> void load<uint64_t>() { throw std::runtime_error("uint64_t"); }
template <> void load<float>() { throw std::runtime_error("float"); }
template <> void load<double>() { throw std::runtime_error("double"); }
} // namespace EventLoader
} // namespace IO
} // namespace Parallel
} // namespace Mantid

#include "MantidParallel/IO/EventLoaderHelpers.h"

using namespace Mantid;
using namespace Parallel;
using namespace Parallel::IO;

namespace {

class FakeDataSource : public NXEventDataSource<int32_t> {
public:
  FakeDataSource(const int numWorkers) : m_numWorkers(numWorkers) {}
  std::unique_ptr<AbstractEventDataPartitioner<int32_t>>
  setBankIndex(const size_t bank) override {
    m_bank = bank;
    auto index = std::vector<int64_t>{0,
                                      100,
                                      100,
                                      300 * static_cast<int64_t>(m_bank + 1),
                                      500 * static_cast<int64_t>(m_bank + 1),
                                      700 * static_cast<int64_t>(m_bank + 1)};
    std::vector<double> time_zero;
    for (size_t i = 0; i < index.size(); ++i)
      time_zero.push_back(static_cast<double>(10 * i + bank));

    // Drift depening on bank to ensure correct offset is used for every bank.
    int64_t time_zero_offset = 123456789 + 1000000 * m_bank;

    return std::make_unique<EventDataPartitioner<int64_t, double, int32_t>>(
        m_numWorkers, PulseTimeGenerator<int64_t, double>{
                          index, time_zero, "second", time_zero_offset});
  }

  void readEventID(int32_t *event_id, size_t start,
                   size_t count) const override {
    // Factor 13 such that there is a gap in the detector IDs between banks.
    for (size_t i = 0; i < count; ++i)
      event_id[i] = static_cast<int32_t>(m_bank * 13 * m_pixelsPerBank +
                                         (start + i) % m_pixelsPerBank);
  }

  void readEventTimeOffset(int32_t *event_time_offset, size_t start,
                           size_t count) const override {
    for (size_t i = 0; i < count; ++i)
      event_time_offset[i] = static_cast<int32_t>(17 * m_bank + start + i);
  }

  std::string readEventTimeOffsetUnit() const override {
    // Using nanosecond implies that EventLoader must convert to microsecond,
    // allowing us to see and test the conversion in action.
    return "nanosecond";
  }

private:
  const int m_numWorkers;
  const size_t m_pixelsPerBank{77};
  size_t m_bank{0};
};

void do_test_load(const Parallel::Communicator &comm, const size_t chunkSize) {
  const std::vector<size_t> bankSizes{111, 1111, 11111};
  Chunker chunker(comm.size(), comm.rank(), bankSizes, chunkSize);
  // FakeDataSource encodes information on bank and position in file into TOF
  // and pulse times, such that we can verify correct mapping.
  FakeDataSource dataSource(comm.size());
  const std::vector<int32_t> bankOffsets{0, 12 * 77, 24 * 77};
  std::vector<std::vector<Types::Event::TofEvent>> eventLists(
      (3 * 77 + comm.size() - 1 - comm.rank()) / comm.size());
  std::vector<std::vector<Types::Event::TofEvent> *> eventListPtrs;
  for (auto &eventList : eventLists)
    eventListPtrs.emplace_back(&eventList);

  EventParser<int32_t> dataSink(comm, chunker.makeWorkerGroups(), bankOffsets,
                                eventListPtrs);
  TS_ASSERT_THROWS_NOTHING(
      (EventLoader::load<int32_t>(chunker, dataSource, dataSink)));

  for (size_t localSpectrumIndex = 0; localSpectrumIndex < eventLists.size();
       ++localSpectrumIndex) {
    size_t globalSpectrumIndex = comm.size() * localSpectrumIndex + comm.rank();
    size_t bank = globalSpectrumIndex / 77;
    size_t pixelInBank = globalSpectrumIndex % 77;
    TS_ASSERT_EQUALS(eventLists[localSpectrumIndex].size(),
                     (bankSizes[bank] + 77 - 1 - pixelInBank) / 77);
    int64_t previousPulseTime{0};
    for (size_t event = 0; event < eventLists[localSpectrumIndex].size();
         ++event) {
      // Every 77th event in the input is in this list so our TOF should jump
      // over 77 TOFs in the input.
      double microseconds =
          static_cast<double>(17 * bank + 77 * event + pixelInBank) * 1e-3;
      TS_ASSERT_EQUALS(eventLists[localSpectrumIndex][event].tof(),
                       microseconds);
      size_t index = event * 77 + pixelInBank;
      size_t pulse = 0;
      if (index >= 100)
        pulse = 2;
      if (index >= 300 * static_cast<size_t>(bank + 1))
        pulse = 3;
      if (index >= 500 * static_cast<size_t>(bank + 1))
        pulse = 4;
      if (index >= 700 * static_cast<size_t>(bank + 1))
        pulse = 5;
      // Testing different aspects that affect pulse time:
      // - `123456789 + 1000000 * bank` confirms that the event_time_zero
      //   offset attribute is taken into account, and for correct bank.
      // - `10 * pulse + bank` confirms that currect event_index is used and
      //   event_time_offset is used correctly, and for correct bank.
      // - The factor 1000000000 converts event_time_offset from input unit
      //   seconds to nanoseconds, confirming that the input unit is adhered to.
      const auto pulseTime =
          eventLists[localSpectrumIndex][event].pulseTime().totalNanoseconds();
      TS_ASSERT_EQUALS(pulseTime, 123456789 + 1000000 * bank +
                                      (10 * pulse + bank) * 1000000000);
      TS_ASSERT(pulseTime >= previousPulseTime);
      previousPulseTime = pulseTime;
    }
  }
}
} // namespace

class EventLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventLoaderTest *createSuite() { return new EventLoaderTest(); }
  static void destroySuite(EventLoaderTest *suite) { delete suite; }

  void test_throws_if_file_does_not_exist() {
    TS_ASSERT_THROWS(
        EventLoader::load(Communicator{}, "abcdefg", "", {}, {}, {}),
        const H5::FileIException &);
  }

  void test_H5DataType_parameter_pack_conversion() {
    using EventLoader::load;
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_INT32),
                            const std::runtime_error &e, std::string(e.what()),
                            "int32_t");
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_INT64),
                            const std::runtime_error &e, std::string(e.what()),
                            "int64_t");
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_UINT32),
                            const std::runtime_error &e, std::string(e.what()),
                            "uint32_t");
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_UINT64),
                            const std::runtime_error &e, std::string(e.what()),
                            "uint64_t");
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_FLOAT),
                            const std::runtime_error &e, std::string(e.what()),
                            "float");
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_DOUBLE),
                            const std::runtime_error &e, std::string(e.what()),
                            "double");
    TS_ASSERT_THROWS_EQUALS(
        load(H5::PredType::NATIVE_CHAR), const std::runtime_error &e,
        std::string(e.what()),
        "Unsupported H5::DataType for event_time_offset in NXevent_data");
  }

  void test_load() {
    for (const size_t chunkSize : {37, 123, 1111}) {
      for (const auto threads : {1, 2, 3, 5, 7, 13}) {
        ParallelTestHelpers::ParallelRunner runner(threads);
        runner.run(do_test_load, chunkSize);
      }
    }
  }
};

#endif /* MANTID_PARALLEL_EVENTLOADERTEST_H_ */
