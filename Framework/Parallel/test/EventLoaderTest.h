#ifndef MANTID_PARALLEL_EVENTLOADERTEST_H_
#define MANTID_PARALLEL_EVENTLOADERTEST_H_

#include <cxxtest/TestSuite.h>

#include <H5Cpp.h>

namespace Mantid {
namespace Parallel {
namespace IO {
namespace EventLoader {
namespace detail {
template <class T> void load() { throw std::runtime_error("unknown"); }
template <> void load<int32_t>() { throw std::runtime_error("int32_t"); }
template <> void load<int64_t>() { throw std::runtime_error("int64_t"); }
template <> void load<uint32_t>() { throw std::runtime_error("uint32_t"); }
template <> void load<uint64_t>() { throw std::runtime_error("uint64_t"); }
template <> void load<float>() { throw std::runtime_error("float"); }
template <> void load<double>() { throw std::runtime_error("double"); }
}
}
}
}
}

#include "MantidParallel/IO/EventLoader.h"

using namespace Mantid::Parallel::IO;

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
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_INT32),
        const std::runtime_error &e, std::string(e.what()), "int32_t");
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_INT64),
        const std::runtime_error &e, std::string(e.what()), "int64_t");
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_UINT32),
        const std::runtime_error &e, std::string(e.what()), "uint32_t");
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_UINT64),
        const std::runtime_error &e, std::string(e.what()), "uint64_t");
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_FLOAT),
        const std::runtime_error &e, std::string(e.what()), "float");
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_DOUBLE),
        const std::runtime_error &e, std::string(e.what()), "double");
    TS_ASSERT_THROWS_EQUALS(
        EventLoader::detail::load(H5::PredType::NATIVE_CHAR),
        const std::runtime_error &e, std::string(e.what()),
        "Unsupported H5::DataType for entry in NXevent_data");
  }

  void test_tmp() {
    EventLoader::load("/home/simon/mantid/nexus/load-performance/sample-files/"
                      "PG3_4871_event.nxs",
                      "entry", {"bank102_events"}, {0}, {nullptr});
    // EventLoader::load("/mnt/extra/simon/neutron-data/realistic_NXEvent_data/events-100000_banks-7_pixels-10000_chunk-262144_compress-None.hdf5",
    //                  "entry/instrument", {"events-0"}, {0}, {nullptr});
  }
};

#endif /* MANTID_PARALLEL_EVENTLOADERTEST_H_ */
