// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMFACTORYTEST_H_
#define ALGORITHMFACTORYTEST_H_

#include "FakeAlgorithms.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/Instantiator.h"
#include <cxxtest/TestSuite.h>

class AlgorithmFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmFactoryTest *createSuite() {
    return new AlgorithmFactoryTest();
  }
  static void destroySuite(AlgorithmFactoryTest *suite) { delete suite; }

  AlgorithmFactoryTest() {}

  ~AlgorithmFactoryTest() override {}

  void testSubscribe() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    auto &algFactory = AlgorithmFactory::Instance();

    // Ensure the algorithm factory does not already have this
    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);

    // get the number of algorithms it already has
    std::vector<std::string> keys = algFactory.getKeys();
    size_t noOfAlgs = keys.size();

    TS_ASSERT_THROWS_NOTHING(algFactory.subscribe<ToyAlgorithm>());
    TS_ASSERT_THROWS_NOTHING(algFactory.subscribe(newTwo));

    TS_ASSERT_THROWS_ANYTHING(algFactory.subscribe<ToyAlgorithm>());

    // get the number of algorithms it has now
    keys = algFactory.getKeys();
    size_t noOfAlgsAfter = keys.size();
    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs + 2);

    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);
  }

  void testUnsubscribe() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    auto &algFactory = AlgorithmFactory::Instance();

    // get the nubmer of algorithms it already has
    std::vector<std::string> keys = algFactory.getKeys();
    size_t noOfAlgs = keys.size();

    algFactory.subscribe<ToyAlgorithm>();
    algFactory.subscribe(newTwo);

    TS_ASSERT_THROWS_NOTHING(algFactory.unsubscribe("ToyAlgorithm", 1));
    TS_ASSERT_THROWS_NOTHING(algFactory.unsubscribe("ToyAlgorithm", 2));

    // get the nubmer of algorithms it has now
    keys = algFactory.getKeys();
    size_t noOfAlgsAfter = keys.size();

    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs)

    // try unsubscribing them again
    TS_ASSERT_THROWS_NOTHING(algFactory.unsubscribe("ToyAlgorithm", 1));
    TS_ASSERT_THROWS_NOTHING(algFactory.unsubscribe("ToyAlgorithm", 2));

    // make sure the number hasn't changed
    keys = algFactory.getKeys();
    size_t noOfAlgsAgain = keys.size();

    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgsAgain);
  }

  void testExists() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    auto &algFactory = AlgorithmFactory::Instance();

    algFactory.subscribe<ToyAlgorithm>();
    algFactory.subscribe(newTwo);

    TS_ASSERT(algFactory.exists("ToyAlgorithm", 1));
    TS_ASSERT(algFactory.exists("ToyAlgorithm", 2));
    TS_ASSERT(!algFactory.exists("ToyAlgorithm", 3));
    TS_ASSERT(!algFactory.exists("ToyAlgorithm", 4));
    TS_ASSERT(algFactory.exists("ToyAlgorithm", -1));

    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);
  }

  void testGetKeys() {
    std::vector<std::string> keys;

    auto &algFactory = AlgorithmFactory::Instance();

    TS_ASSERT_EQUALS(0, keys.size());
    algFactory.subscribe<ToyAlgorithm>();

    TS_ASSERT_THROWS_NOTHING(keys = algFactory.getKeys());
    size_t noOfAlgs = keys.size();

    algFactory.unsubscribe("ToyAlgorithm", 1);

    TS_ASSERT_THROWS_NOTHING(keys = algFactory.getKeys());
    TS_ASSERT_EQUALS(noOfAlgs - 1, keys.size());
  }

  void test_HighestVersion() {
    auto &algFactory = AlgorithmFactory::Instance();

    TS_ASSERT_THROWS(algFactory.highestVersion("ToyAlgorithm"),
                     const std::invalid_argument &);

    algFactory.subscribe<ToyAlgorithm>();
    TS_ASSERT_EQUALS(1, algFactory.highestVersion("ToyAlgorithm"));

    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;
    algFactory.subscribe(newTwo);
    TS_ASSERT_EQUALS(2, algFactory.highestVersion("ToyAlgorithm"));

    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);
  }

  void testCreate() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    auto &algFactory = AlgorithmFactory::Instance();

    algFactory.subscribe<ToyAlgorithm>();
    algFactory.subscribe(newTwo);

    TS_ASSERT_THROWS_NOTHING(algFactory.create("ToyAlgorithm", -1));
    TS_ASSERT_THROWS_ANYTHING(algFactory.create("AlgorithmDoesntExist", -1));

    TS_ASSERT_THROWS_NOTHING(algFactory.create("ToyAlgorithm", 1));
    TS_ASSERT_THROWS_NOTHING(algFactory.create("ToyAlgorithm", 2));
    TS_ASSERT_THROWS_ANYTHING(algFactory.create("AlgorithmDoesntExist", 1));
    TS_ASSERT_THROWS_ANYTHING(algFactory.create("AlgorithmDoesntExist", 2));

    TS_ASSERT_THROWS_ANYTHING(algFactory.create("", 1));
    TS_ASSERT_THROWS_ANYTHING(algFactory.create("", -1));

    TS_ASSERT_THROWS_ANYTHING(algFactory.create("ToyAlgorithm", 3));
    TS_ASSERT_THROWS_ANYTHING(algFactory.create("ToyAlgorithm", 4));

    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);
  }

  void testGetDescriptors() {
    auto &algFactory = AlgorithmFactory::Instance();

    algFactory.subscribe<ToyAlgorithm>();
    std::vector<AlgorithmDescriptor> descriptors;
    TS_ASSERT_THROWS_NOTHING(descriptors = algFactory.getDescriptors(true));

    size_t noOfAlgs = descriptors.size();
    std::vector<AlgorithmDescriptor>::const_iterator descItr =
        descriptors.begin();
    bool foundAlg = false;
    while (descItr != descriptors.end() && !foundAlg) {
      foundAlg = ("Cat" == descItr->category) &&
                 ("ToyAlgorithm" == descItr->name) &&
                 ("Dog" == descItr->alias) && (1 == descItr->version);
      descItr++;
    }
    TS_ASSERT(foundAlg);

    algFactory.unsubscribe("ToyAlgorithm", 1);

    TS_ASSERT_THROWS_NOTHING(descriptors = algFactory.getDescriptors(true));

    TS_ASSERT_EQUALS(noOfAlgs - 1, descriptors.size());
  }

  void testGetCategories() {
    auto &algFactory = AlgorithmFactory::Instance();
    algFactory.subscribe<CategoryAlgorithm>();
    std::unordered_set<std::string> validCategories;
    TS_ASSERT_THROWS_NOTHING(validCategories = algFactory.getCategories(true));

    size_t noOfCats = validCategories.size();
    TS_ASSERT_DIFFERS(validCategories.find("Fake"), validCategories.end());

    algFactory.unsubscribe("CategoryAlgorithm", 1);
    TS_ASSERT_THROWS_NOTHING(validCategories = algFactory.getCategories(true));
    TS_ASSERT_EQUALS(noOfCats - 1, validCategories.size());
  }

  void testGetCategoriesWithState() {
    auto &algFactory = AlgorithmFactory::Instance();
    algFactory.subscribe<CategoryAlgorithm>();

    std::map<std::string, bool> validCategories;
    TS_ASSERT_THROWS_NOTHING(validCategories =
                                 algFactory.getCategoriesWithState());
    size_t noOfCats = validCategories.size();
    TS_ASSERT_DIFFERS(validCategories.find("Fake"), validCategories.end());

    algFactory.unsubscribe("CategoryAlgorithm", 1);
    TS_ASSERT_THROWS_NOTHING(validCategories =
                                 algFactory.getCategoriesWithState());
    TS_ASSERT_EQUALS(noOfCats - 1, validCategories.size());
  }

  void testDecodeName() {
    auto &algFactory = AlgorithmFactory::Instance();
    std::pair<std::string, int> basePair;
    basePair.first = "Cat";
    basePair.second = 1;
    std::string mangledName = "Cat|1";
    std::pair<std::string, int> outPair;
    TS_ASSERT_THROWS_NOTHING(outPair = algFactory.decodeName(mangledName));
    TS_ASSERT_EQUALS(basePair.first, outPair.first);
    TS_ASSERT_EQUALS(basePair.second, outPair.second);

    mangledName = "Cat 1";
    TS_ASSERT_THROWS_ANYTHING(outPair = algFactory.decodeName(mangledName));
  }
};

#endif
