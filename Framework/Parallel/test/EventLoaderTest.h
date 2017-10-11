#ifndef MANTID_PARALLEL_EVENTLOADERTEST_H_
#define MANTID_PARALLEL_EVENTLOADERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/EventDataSink.h"
#include "MantidParallel/IO/EventLoader.h"
#include "MantidParallel/IO/NXEventDataSource.h"

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
template <class T1, class T2> void load() {
  throw std::runtime_error("unknown");
}
template <> void load<int32_t, float>() { throw std::runtime_error("float"); }
template <> void load<int32_t, double>() { throw std::runtime_error("double"); }
}
}
}
}

#include "MantidParallel/IO/EventLoaderHelpers.h"

using namespace Mantid::Parallel::IO;

namespace detail {
class FakeDataSource : public NXEventDataSource<int64_t, int64_t, int32_t> {
public:
  void setBankIndex(const size_t bank) override {
    m_bank = bank;
    m_index = std::vector<int64_t>{0, 2, 4, 8};
    m_time_zero.clear();
    for (size_t i = 0; i < m_index.size(); ++i)
      m_time_zero.push_back(static_cast<int64_t>(1000 * i + bank));
  }

  const std::vector<int64_t> &eventIndex() const override { return m_index; }
  const std::vector<int64_t> &eventTimeZero() const override {
    return m_time_zero;
  }
  int64_t eventTimeZeroOffset() const override { return 123456789; }
  void readEventID(int32_t *event_id, size_t start,
                   size_t count) const override {
    for (size_t i = 0; i < count; ++i)
      event_id[i] = static_cast<int32_t>(m_bank * m_bankSize +
                                         (start + count) % m_bankSize);
  }
  void readEventTimeOffset(int32_t *event_time_offset, size_t start,
                           size_t count) const override {
    for (size_t i = 0; i < count; ++i)
      event_time_offset[i] = static_cast<int32_t>(start + count);
  }

private:
  const size_t m_bankSize{10};
  size_t m_bank;
  std::vector<int64_t> m_index;
  std::vector<int64_t> m_time_zero;
};

class FakeDataSink : public EventDataSink<int64_t, int64_t, int32_t> {
public:
  void setPulseInformation(std::vector<int64_t> event_index,
                           std::vector<int64_t> event_time_zero,
                           const int64_t event_time_zero_offset) override {
    static_cast<void>(event_index);
    static_cast<void>(event_time_zero);
    static_cast<void>(event_time_zero_offset);
  }
  void startAsync(int32_t *event_id_start,
                  const int32_t *event_time_offset_start,
                  const Chunker::LoadRange &range) override {
    static_cast<void>(event_id_start);
    static_cast<void>(event_time_offset_start);
    static_cast<void>(range);
  }
  void wait() const override {}
};
}

class EventLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventLoaderTest *createSuite() { return new EventLoaderTest(); }
  static void destroySuite(EventLoaderTest *suite) { delete suite; }

  void test_throws_if_file_does_not_exist() {
    TS_ASSERT_THROWS(EventLoader::load("abcdefg", "", {}, {}, {}),
                     H5::FileIException);
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
    // Only integers accepted for first argument since this is the event index.
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_FLOAT),
                            const std::runtime_error &e, std::string(e.what()),
                            "Unsupported H5::DataType for event_index in "
                            "NXevent_data, must be integer");
    TS_ASSERT_THROWS_EQUALS(load(H5::PredType::NATIVE_DOUBLE),
                            const std::runtime_error &e, std::string(e.what()),
                            "Unsupported H5::DataType for event_index in "
                            "NXevent_data, must be integer");
    // Other arguments (event_time_zero and event_time_offset) can be floats.
    TS_ASSERT_THROWS_EQUALS(
        load(H5::PredType::NATIVE_INT32, H5::PredType::NATIVE_FLOAT),
        const std::runtime_error &e, std::string(e.what()), "float");
    TS_ASSERT_THROWS_EQUALS(
        load(H5::PredType::NATIVE_INT32, H5::PredType::NATIVE_DOUBLE),
        const std::runtime_error &e, std::string(e.what()), "double");
    TS_ASSERT_THROWS_EQUALS(
        load(H5::PredType::NATIVE_INT32, H5::PredType::NATIVE_CHAR),
        const std::runtime_error &e, std::string(e.what()),
        "Unsupported H5::DataType for entry in NXevent_data");
  }

  void test_load() {
    const std::vector<size_t> bankSizes{10, 100, 1000};
    const size_t chunkSize{37};
    Chunker chunker(1, 0, bankSizes, chunkSize);
    ::detail::FakeDataSource dataSource;
    ::detail::FakeDataSink dataSink;
    TS_ASSERT_THROWS_NOTHING((EventLoader::load<int64_t, int64_t, int32_t>(
        chunker, dataSource, dataSink)));
    // TODO cannot test anything useful before we have the parser.
  }
};

#endif /* MANTID_PARALLEL_EVENTLOADERTEST_H_ */
