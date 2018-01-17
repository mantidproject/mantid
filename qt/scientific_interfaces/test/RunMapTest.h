#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_

#include "../EnggDiffraction/RunMap.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class RunMapTest : public CxxTest::TestSuite {

public:
  void test_addedItemsExistInMap() {
    RunMap<3, std::string> runMap;
    TS_ASSERT_THROWS_NOTHING(runMap.add(123, 1, "Polly"));
    TS_ASSERT_THROWS_NOTHING(runMap.add(456, 2, "Morphism"));
    TS_ASSERT_THROWS(runMap.add(789, 4, "Al"), std::invalid_argument);

    TS_ASSERT(runMap.contains(123, 1));
    TS_ASSERT(runMap.contains(456, 2));
    TS_ASSERT(!runMap.contains(789, 4));
  }

  void test_addedItemsAreCorrect() {
    RunMap<3, std::string> runMap;
    runMap.add(123, 1, "Polly");
    runMap.add(456, 2, "Morphism");

    TS_ASSERT_EQUALS(runMap.get(123, 1), "Polly");
    TS_ASSERT_EQUALS(runMap.get(456, 2), "Morphism");
  }

  void test_remove() {
    RunMap<3, std::string> runMap;

    runMap.add(123, 1, "Polly");
    TS_ASSERT(runMap.contains(123, 1));

    TS_ASSERT_THROWS_NOTHING(runMap.remove(123, 1));
    TS_ASSERT(!runMap.contains(123, 1));
  }

  void test_getRunNumbersAndBankIDs() {
    RunMap<3, std::string> runMap;

    runMap.add(111, 1, "Polly");
    runMap.add(222, 2, "Morphism");
    runMap.add(333, 3, "Al");
    runMap.add(444, 1, "Gorithm");

    std::vector<std::pair<int, size_t>> runBankPairs;
    TS_ASSERT_THROWS_NOTHING(runBankPairs = runMap.getRunNumbersAndBankIDs());

    TS_ASSERT_EQUALS(runBankPairs.size(), 4);
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(runBankPairs[i],
                       std::make_pair((i + 1) * 111, size_t(i % 3 + 1)));
    }
  }
};

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_
