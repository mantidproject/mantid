// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_

#include "../EnggDiffraction/RunMap.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class RunMapTest : public CxxTest::TestSuite {

public:
  void test_addedItemsExistInMap() {
    RunMap<3, std::string> runMap;

    const RunLabel polly("123", 1);
    TS_ASSERT_THROWS_NOTHING(runMap.add(polly, "Polly"));

    const RunLabel morphism("456", 2);
    TS_ASSERT_THROWS_NOTHING(runMap.add(morphism, "Morphism"));

    const RunLabel al("789", 4);
    TS_ASSERT_THROWS(runMap.add(al, "Al"), const std::invalid_argument &);

    TS_ASSERT(runMap.contains(polly));
    TS_ASSERT(runMap.contains(morphism));
    TS_ASSERT(!runMap.contains(al));
  }

  void test_addedItemsAreCorrect() {
    RunMap<3, std::string> runMap;

    const RunLabel polly("123", 1);
    runMap.add(polly, "Polly");

    const RunLabel morphism("456", 2);
    runMap.add(morphism, "Morphism");

    TS_ASSERT_EQUALS(runMap.get(polly), "Polly");
    TS_ASSERT_EQUALS(runMap.get(morphism), "Morphism");
  }

  void test_remove() {
    RunMap<3, std::string> runMap;

    const RunLabel polly("123", 1);
    runMap.add(polly, "Polly");
    TS_ASSERT(runMap.contains(polly));

    TS_ASSERT_THROWS_NOTHING(runMap.remove(polly));
    TS_ASSERT(!runMap.contains(polly));

    const RunLabel invalid("123", 4);
    TS_ASSERT_THROWS(runMap.remove(invalid), const std::invalid_argument &);
  }

  void test_getRunLabels() {
    RunMap<3, std::string> runMap;

    const RunLabel polly("111", 0);
    runMap.add(polly, "Polly");

    const RunLabel morphism("222", 1);
    runMap.add(morphism, "Morphism");

    const RunLabel al("333", 2);
    runMap.add(al, "Al");

    const RunLabel gorithm("444", 0);
    runMap.add(gorithm, "Gorithm");

    const std::vector<RunLabel> runLabels({polly, morphism, al, gorithm});

    std::vector<RunLabel> retrievedRunLabels;
    TS_ASSERT_THROWS_NOTHING(retrievedRunLabels = runMap.getRunLabels());

    TS_ASSERT_EQUALS(retrievedRunLabels.size(), 4);
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(runLabels[i], retrievedRunLabels[i]);
    }
  }

  void test_size() {
    RunMap<3, std::string> runMap;
    TS_ASSERT_EQUALS(runMap.size(), 0);

    runMap.add(RunLabel("111", 0), "Polly");
    runMap.add(RunLabel("222", 1), "Morphism");
    TS_ASSERT_EQUALS(runMap.size(), 2);

    runMap.add(RunLabel("333", 2), "Al");
    runMap.add(RunLabel("444", 0), "Gorithm");
    TS_ASSERT_EQUALS(runMap.size(), 4);
  }
};

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAPTEST_H_
