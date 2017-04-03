#ifndef MANTID_INDEXING_INDEXINFOTEST_H_
#define MANTID_INDEXING_INDEXINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/make_cow.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid;
using namespace Indexing;

class IndexInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexInfoTest *createSuite() { return new IndexInfoTest(); }
  static void destroySuite(IndexInfoTest *suite) { delete suite; }

  void test_size_constructor() { TS_ASSERT_THROWS_NOTHING(IndexInfo(3)); }

  void test_size_constructor_sets_correct_indices() {
    IndexInfo info(3);
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 3);
    TS_ASSERT(!info.spectrumDefinitions());
  }

  void test_vector_constructor() {
    TS_ASSERT_THROWS_NOTHING(IndexInfo({3, 2, 1}));
  }

  void test_vector_constructor_sets_correct_indices() {
    IndexInfo info({3, 2, 1});
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 1);
  }

  void test_size() { TS_ASSERT_EQUALS(IndexInfo(3).size(), 3); }

  void test_copy() {
    IndexInfo info({3, 2, 1});
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    info.setSpectrumDefinitions(defs);
    auto copy(info);
    TS_ASSERT_EQUALS(info.size(), 3);
    TS_ASSERT_EQUALS(copy.size(), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(copy.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumDefinitions(), copy.spectrumDefinitions());
  }

  void test_move() {
    IndexInfo info({3, 2, 1});
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    info.setSpectrumDefinitions(defs);
    auto moved(std::move(info));
    TS_ASSERT_EQUALS(info.size(), 0);
    TS_ASSERT_EQUALS(moved.size(), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(moved.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumDefinitions(), nullptr);
  }

  void test_setSpectrumNumbers_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setSpectrumNumbers({1, 2}), std::runtime_error);
  }

  void test_setSpectrumDefinitions_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_EQUALS(
        t.setSpectrumDefinitions(std::vector<SpectrumDefinition>(2)),
        const std::runtime_error &e, std::string(e.what()),
        "IndexInfo: Size mismatch when setting new spectrum definitions");
  }

  void test_setSpectrumNumbers() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumNumbers({3, 4, 5}));
    TS_ASSERT_EQUALS(t.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(t.spectrumNumber(1), 4);
    TS_ASSERT_EQUALS(t.spectrumNumber(2), 5);
  }

  void test_setSpectrumDefinitions() {
    IndexInfo t(3);
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[2].add(8);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumDefinitions(specDefs));
    TS_ASSERT(t.spectrumDefinitions());
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[0], specDefs[0]);
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[1], specDefs[1]);
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[2], specDefs[2]);
  }

  void test_setSpectrumDefinitions_setting_nullptr_fails() {
    // This might be supported in the future but is not needed now and might
    // break some things, so we forbid this for now.
    IndexInfo info(3);
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> defs{nullptr};
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), std::runtime_error);
  }

  void test_setSpectrumDefinitions_size_mismatch_cow_ptr() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(2);
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), std::runtime_error);
  }

  void test_setSpectrumDefinitions_cow_ptr() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    TS_ASSERT_THROWS_NOTHING(info.setSpectrumDefinitions(defs));
    TS_ASSERT_EQUALS(info.spectrumDefinitions().get(), defs.get());
  }

  void test_StorageMode_Cloned() {
    int nrank = 2;
    IndexInfo rank0(3, IndexInfo::StorageMode::Cloned,
                    IndexInfo::Communicator{nrank, 0});
    IndexInfo rank1(3, IndexInfo::StorageMode::Cloned,
                    IndexInfo::Communicator{nrank, 1});
    TS_ASSERT_EQUALS(rank0.size(), 3);
    TS_ASSERT_EQUALS(rank0.globalSize(), 3);
    TS_ASSERT_EQUALS(rank0.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(rank0.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(rank0.spectrumNumber(2), 3);
    TS_ASSERT_EQUALS(rank1.size(), 3);
    TS_ASSERT_EQUALS(rank1.globalSize(), 3);
    TS_ASSERT_EQUALS(rank1.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(rank1.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(rank1.spectrumNumber(2), 3);
  }

  void test_StorageMode_Distributed() {
    int nrank = 2;
    IndexInfo rank0(3, IndexInfo::StorageMode::Distributed,
                    IndexInfo::Communicator{nrank, 0});
    IndexInfo rank1(3, IndexInfo::StorageMode::Distributed,
                    IndexInfo::Communicator{nrank, 1});
    // Current default is RoundRobinPartitioner
    TS_ASSERT_EQUALS(rank0.size(), 2);
    TS_ASSERT_EQUALS(rank0.globalSize(), 3);
    TS_ASSERT_EQUALS(rank0.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(rank0.spectrumNumber(1), 3);
    TS_ASSERT_EQUALS(rank1.size(), 1);
    TS_ASSERT_EQUALS(rank1.globalSize(), 3);
    TS_ASSERT_EQUALS(rank1.spectrumNumber(0), 2);
  }

  void test_StorageMode_MasterOnly() {
    int nrank = 2;
    TS_ASSERT_THROWS(IndexInfo(3, IndexInfo::StorageMode::MasterOnly,
                               IndexInfo::Communicator{nrank, 0}),
                     std::runtime_error);
  }

  void test_isOnThisPartition_StorageMode_Cloned() {
    int nrank = 2;
    IndexInfo rank0(3, IndexInfo::StorageMode::Cloned,
                    IndexInfo::Communicator{nrank, 0});
    IndexInfo rank1(3, IndexInfo::StorageMode::Cloned,
                    IndexInfo::Communicator{nrank, 1});
    for (size_t i = 0; i < rank0.globalSize(); ++i)
      TS_ASSERT(rank0.isOnThisPartition(i));
    for (size_t i = 0; i < rank1.globalSize(); ++i)
      TS_ASSERT(rank1.isOnThisPartition(i));
  }

  void test_isOnThisPartition_StorageMode_Distributed() {
    int nrank = 2;
    IndexInfo rank0(3, IndexInfo::StorageMode::Distributed,
                    IndexInfo::Communicator{nrank, 0});
    IndexInfo rank1(3, IndexInfo::StorageMode::Distributed,
                    IndexInfo::Communicator{nrank, 1});
    // Current default is RoundRobinPartitioner
    for (size_t i = 0; i < rank0.globalSize(); ++i) {
      if (i % nrank == 0) {
        TS_ASSERT(rank0.isOnThisPartition(i));
      } else {
        TS_ASSERT(rank1.isOnThisPartition(i));
      }
    }
  }
};

#endif /* MANTID_INDEXING_INDEXINFOTEST_H_ */
