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

    // get the number of algorithms it already has
    std::vector<std::string> keys = AlgorithmFactory::Instance().getKeys();
    size_t noOfAlgs = keys.size();

    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().subscribe<ToyAlgorithm>());
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe(newTwo));

    TS_ASSERT_THROWS_ANYTHING(
        AlgorithmFactory::Instance().subscribe<ToyAlgorithm>());

    // get the number of algorithms it has now
    keys = AlgorithmFactory::Instance().getKeys();
    size_t noOfAlgsAfter = keys.size();
    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs + 2);

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 2);
  }

  void testUnsubscribe() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    // get the nubmer of algorithms it already has
    std::vector<std::string> keys = AlgorithmFactory::Instance().getKeys();
    size_t noOfAlgs = keys.size();

    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    AlgorithmFactory::Instance().subscribe(newTwo);

    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1));
    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 2));

    // get the nubmer of algorithms it has now
    keys = AlgorithmFactory::Instance().getKeys();
    size_t noOfAlgsAfter = keys.size();

    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs)

    // try unsubscribing them again
    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1));
    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 2));

    // make sure the number hasn't changed
    keys = AlgorithmFactory::Instance().getKeys();
    size_t noOfAlgsAgain = keys.size();

    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgsAgain);
  }

  void testExists() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    AlgorithmFactory::Instance().subscribe(newTwo);

    TS_ASSERT(AlgorithmFactory::Instance().exists("ToyAlgorithm", 1));
    TS_ASSERT(AlgorithmFactory::Instance().exists("ToyAlgorithm", 2));
    TS_ASSERT(!AlgorithmFactory::Instance().exists("ToyAlgorithm", 3));
    TS_ASSERT(!AlgorithmFactory::Instance().exists("ToyAlgorithm", 4));
    TS_ASSERT(AlgorithmFactory::Instance().exists("ToyAlgorithm", -1));

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 2);
  }

  void testGetKeys() {
    std::vector<std::string> keys;

    TS_ASSERT_EQUALS(0, keys.size());
    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();

    TS_ASSERT_THROWS_NOTHING(keys = AlgorithmFactory::Instance().getKeys());
    size_t noOfAlgs = keys.size();

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);

    TS_ASSERT_THROWS_NOTHING(keys = AlgorithmFactory::Instance().getKeys());
    TS_ASSERT_EQUALS(noOfAlgs - 1, keys.size());
  }

  void test_HighestVersion() {
    auto &factory = AlgorithmFactory::Instance();

    TS_ASSERT_THROWS(factory.highestVersion("ToyAlgorithm"),
                     std::invalid_argument);

    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    TS_ASSERT_EQUALS(1, factory.highestVersion("ToyAlgorithm"));

    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;
    AlgorithmFactory::Instance().subscribe(newTwo);
    TS_ASSERT_EQUALS(2, factory.highestVersion("ToyAlgorithm"));

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 2);
  }

  void testCreate() {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm> *newTwo =
        new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    AlgorithmFactory::Instance().subscribe(newTwo);

    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().create("ToyAlgorithm", -1));
    TS_ASSERT_THROWS_ANYTHING(
        AlgorithmFactory::Instance().create("AlgorithmDoesntExist", -1));

    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().create("ToyAlgorithm", 1));
    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().create("ToyAlgorithm", 2));
    TS_ASSERT_THROWS_ANYTHING(
        AlgorithmFactory::Instance().create("AlgorithmDoesntExist", 1));
    TS_ASSERT_THROWS_ANYTHING(
        AlgorithmFactory::Instance().create("AlgorithmDoesntExist", 2));

    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("", 1));
    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("", -1));

    TS_ASSERT_THROWS_ANYTHING(
        AlgorithmFactory::Instance().create("ToyAlgorithm", 3));
    TS_ASSERT_THROWS_ANYTHING(
        AlgorithmFactory::Instance().create("ToyAlgorithm", 4));

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 2);
  }

  void testGetDescriptors() {
    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    std::vector<AlgorithmDescriptor> descriptors;
    TS_ASSERT_THROWS_NOTHING(
        descriptors = AlgorithmFactory::Instance().getDescriptors(true));

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

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);

    TS_ASSERT_THROWS_NOTHING(
        descriptors = AlgorithmFactory::Instance().getDescriptors(true));

    TS_ASSERT_EQUALS(noOfAlgs - 1, descriptors.size());
  }

  void testGetCategories() {
    AlgorithmFactory::Instance().subscribe<CategoryAlgorithm>();
    std::unordered_set<std::string> validCategories;
    TS_ASSERT_THROWS_NOTHING(
        validCategories = AlgorithmFactory::Instance().getCategories(true));

    size_t noOfCats = validCategories.size();
    TS_ASSERT_DIFFERS(validCategories.find("Fake"), validCategories.end());

    AlgorithmFactory::Instance().unsubscribe("CategoryAlgorithm", 1);
    TS_ASSERT_THROWS_NOTHING(
        validCategories = AlgorithmFactory::Instance().getCategories(true));
    TS_ASSERT_EQUALS(noOfCats - 1, validCategories.size());
  }

  void testGetCategoriesWithState() {
    AlgorithmFactory::Instance().subscribe<CategoryAlgorithm>();

    std::map<std::string, bool> validCategories;
    TS_ASSERT_THROWS_NOTHING(
        validCategories =
            AlgorithmFactory::Instance().getCategoriesWithState());
    size_t noOfCats = validCategories.size();
    TS_ASSERT_DIFFERS(validCategories.find("Fake"), validCategories.end());

    AlgorithmFactory::Instance().unsubscribe("CategoryAlgorithm", 1);
    TS_ASSERT_THROWS_NOTHING(
        validCategories =
            AlgorithmFactory::Instance().getCategoriesWithState());
    TS_ASSERT_EQUALS(noOfCats - 1, validCategories.size());
  }

  void testDecodeName() {
    std::pair<std::string, int> basePair;
    basePair.first = "Cat";
    basePair.second = 1;
    std::string mangledName = "Cat|1";
    std::pair<std::string, int> outPair;
    TS_ASSERT_THROWS_NOTHING(
        outPair = AlgorithmFactory::Instance().decodeName(mangledName));
    TS_ASSERT_EQUALS(basePair.first, outPair.first);
    TS_ASSERT_EQUALS(basePair.second, outPair.second);

    mangledName = "Cat 1";
    TS_ASSERT_THROWS_ANYTHING(
        outPair = AlgorithmFactory::Instance().decodeName(mangledName));
  }
};

#endif
