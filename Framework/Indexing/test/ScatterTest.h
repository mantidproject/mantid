#ifndef MANTID_INDEXING_SCATTERTEST_H_
#define MANTID_INDEXING_SCATTERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/Scatter.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidParallel/Communicator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include "MantidTestHelpers/ParallelRunner.h"

using namespace Mantid;
using namespace Mantid::Indexing;
using namespace Mantid::Parallel;
using namespace ParallelTestHelpers;

namespace {
IndexInfo makeIndexInfo(const Communicator &comm = Communicator()) {
  IndexInfo indexInfo(7, StorageMode::Cloned, comm);
  std::vector<SpectrumDefinition> specDefs;
  specDefs.emplace_back(1);
  specDefs.emplace_back(2);
  specDefs.emplace_back(4);
  specDefs.emplace_back(8);
  specDefs.emplace_back();
  specDefs.emplace_back();
  specDefs.emplace_back();
  indexInfo.setSpectrumDefinitions(std::move(specDefs));
  return indexInfo;
}

void run_StorageMode_Cloned(const Communicator &comm) {
  const auto indexInfo = makeIndexInfo(comm);
  if (comm.size() == 1) {
    TS_ASSERT_THROWS_NOTHING(scatter(indexInfo));
  } else {
    const auto result = scatter(indexInfo);
    TS_ASSERT_EQUALS(result.storageMode(), StorageMode::Distributed);
    TS_ASSERT_EQUALS(result.globalSize(), indexInfo.size());
    // Assuming round-robin partitioning
    TS_ASSERT_EQUALS(result.size(),
                     (indexInfo.size() + comm.size() - 1 - comm.rank()) /
                         comm.size());
    const auto resultSpecDefs = result.spectrumDefinitions();
    const auto specDefs = indexInfo.spectrumDefinitions();
    const auto indices = result.makeIndexSet();
    size_t current = 0;
    for (size_t i = 0; i < specDefs->size(); ++i) {
      if (static_cast<int>(i) % comm.size() == comm.rank()) {
        TS_ASSERT_EQUALS(result.spectrumNumber(indices[current]),
                         indexInfo.spectrumNumber(i));
        TS_ASSERT_EQUALS(resultSpecDefs->at(indices[current]), specDefs->at(i));
        ++current;
      }
    }
  }
}
} // namespace

class ScatterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScatterTest *createSuite() { return new ScatterTest(); }
  static void destroySuite(ScatterTest *suite) { delete suite; }

  void test_1_rank() {
    const auto indexInfo = makeIndexInfo();
    const auto result = scatter(indexInfo);
    TS_ASSERT_EQUALS(result.storageMode(), StorageMode::Cloned);
    TS_ASSERT_EQUALS(result.globalSize(), indexInfo.size());
    TS_ASSERT_EQUALS(result.size(), indexInfo.size());
    TS_ASSERT_EQUALS(result.spectrumDefinitions(),
                     indexInfo.spectrumDefinitions());
    TS_ASSERT_EQUALS(result.spectrumNumber(0), indexInfo.spectrumNumber(0));
    TS_ASSERT_EQUALS(result.spectrumNumber(1), indexInfo.spectrumNumber(1));
    TS_ASSERT_EQUALS(result.spectrumNumber(2), indexInfo.spectrumNumber(2));
    TS_ASSERT_EQUALS(result.spectrumNumber(3), indexInfo.spectrumNumber(3));
    TS_ASSERT_EQUALS(result.spectrumNumber(4), indexInfo.spectrumNumber(4));
    TS_ASSERT_EQUALS(result.spectrumNumber(5), indexInfo.spectrumNumber(5));
    TS_ASSERT_EQUALS(result.spectrumNumber(6), indexInfo.spectrumNumber(6));
  }

  void test_StorageMode_Cloned() { runParallel(run_StorageMode_Cloned); }
};

#endif /* MANTID_INDEXING_SCATTERTEST_H_ */
