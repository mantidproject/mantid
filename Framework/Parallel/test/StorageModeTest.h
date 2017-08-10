#ifndef MANTID_PARALLEL_STORAGEMODETEST_H_
#define MANTID_PARALLEL_STORAGEMODETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/StorageMode.h"

using namespace Mantid::Parallel;

class StorageModeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StorageModeTest *createSuite() { return new StorageModeTest(); }
  static void destroySuite(StorageModeTest *suite) { delete suite; }

  void test_toString() {
    TS_ASSERT_EQUALS(toString(StorageMode::Cloned),
                     "Parallel::StorageMode::Cloned");
    TS_ASSERT_EQUALS(toString(StorageMode::Distributed),
                     "Parallel::StorageMode::Distributed");
    TS_ASSERT_EQUALS(toString(StorageMode::MasterOnly),
                     "Parallel::StorageMode::MasterOnly");
  }

  void test_toString_map() {
    std::map<std::string, StorageMode> map;
    map["A"] = StorageMode::Cloned;
    map["B"] = StorageMode::Distributed;
    TS_ASSERT_EQUALS(toString(map), "\nA Parallel::StorageMode::Cloned\nB "
                                    "Parallel::StorageMode::Distributed\n");
  }

  void test_fromString() {
    TS_ASSERT_EQUALS(fromString("Parallel::StorageMode::Cloned"),
                     StorageMode::Cloned);
    TS_ASSERT_EQUALS(fromString("Parallel::StorageMode::Distributed"),
                     StorageMode::Distributed);
    TS_ASSERT_EQUALS(fromString("Parallel::StorageMode::MasterOnly"),
                     StorageMode::MasterOnly);
    TS_ASSERT_THROWS_EQUALS(fromString(""), const std::invalid_argument &e,
                            std::string(e.what()),
                            "Parallel::fromString could not convert provided "
                            "input into a Parallel::StorageMode.");
  }

  void test_fromString_toString() {
    TS_ASSERT_EQUALS(fromString(toString(StorageMode::Cloned)),
                     StorageMode::Cloned);
    TS_ASSERT_EQUALS(fromString(toString(StorageMode::Distributed)),
                     StorageMode::Distributed);
    TS_ASSERT_EQUALS(fromString(toString(StorageMode::MasterOnly)),
                     StorageMode::MasterOnly);
  }
};

#endif /* MANTID_PARALLEL_STORAGEMODETEST_H_ */
