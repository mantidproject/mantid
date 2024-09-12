// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FakeAlgorithms.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/Instantiator.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <optional>

class AlgorithmFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmFactoryTest *createSuite() { return new AlgorithmFactoryTest(); }
  static void destroySuite(AlgorithmFactoryTest *suite) { delete suite; }

  AlgorithmFactoryTest() {}

  ~AlgorithmFactoryTest() override = default;

  void testSubscribe() {
    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newTwo =
        std::make_unique<Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>>();

    auto &algFactory = AlgorithmFactory::Instance();

    // Ensure the algorithm factory does not already have this
    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);

    // get the number of algorithms it already has
    std::vector<std::string> keys = algFactory.getKeys();
    size_t noOfAlgs = keys.size();

    TS_ASSERT_THROWS_NOTHING(algFactory.subscribe<ToyAlgorithm>());
    TS_ASSERT_THROWS_NOTHING(algFactory.subscribe(std::move(newTwo)));

    TS_ASSERT_THROWS_ANYTHING(algFactory.subscribe<ToyAlgorithm>());

    // get the number of algorithms it has now
    keys = algFactory.getKeys();
    size_t noOfAlgsAfter = keys.size();
    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs + 2);

    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);
  }

  void testUnsubscribe() {
    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newTwo =
        std::make_unique<Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>>();

    auto &algFactory = AlgorithmFactory::Instance();

    // get the nubmer of algorithms it already has
    std::vector<std::string> keys = algFactory.getKeys();
    size_t noOfAlgs = keys.size();

    algFactory.subscribe<ToyAlgorithm>();
    algFactory.subscribe(std::move(newTwo));

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
    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newTwo =
        std::make_unique<Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>>();

    auto &algFactory = AlgorithmFactory::Instance();

    algFactory.subscribe<ToyAlgorithm>();
    algFactory.subscribe(std::move(newTwo));

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

  void test_getRealNameFromAlias() {
    auto &algFactory = AlgorithmFactory::Instance();

    const auto resultAlias = algFactory.getRealNameFromAlias("Dog");
    const auto resultFakeAlias = algFactory.getRealNameFromAlias("Frog");

    TS_ASSERT_EQUALS(resultAlias->first, "ToyAlgorithm");
    TS_ASSERT_EQUALS(resultAlias->second, 1);
    TS_ASSERT(!resultFakeAlias);
  }

  void test_HighestVersion() {
    auto &algFactory = AlgorithmFactory::Instance();

    TS_ASSERT_THROWS(algFactory.highestVersion("ToyAlgorithm"), const std::runtime_error &);

    algFactory.subscribe<ToyAlgorithm>();
    TS_ASSERT_EQUALS(1, algFactory.highestVersion("ToyAlgorithm"));
    TS_ASSERT_EQUALS(1, algFactory.highestVersion("Dog"));

    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newTwo =
        std::make_unique<Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>>();
    algFactory.subscribe(std::move(newTwo));
    TS_ASSERT_EQUALS(2, algFactory.highestVersion("ToyAlgorithm"));
    TS_ASSERT_EQUALS(2, algFactory.highestVersion("Dog"));

    algFactory.unsubscribe("ToyAlgorithm", 1);
    algFactory.unsubscribe("ToyAlgorithm", 2);
  }

  void testCreate() {
    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newTwo =
        std::make_unique<Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>>();

    auto &algFactory = AlgorithmFactory::Instance();

    algFactory.subscribe<ToyAlgorithm>();
    algFactory.subscribe(std::move(newTwo));

    TS_ASSERT_THROWS_NOTHING(algFactory.create("ToyAlgorithm", -1));
    TS_ASSERT_THROWS_NOTHING(algFactory.create("Dog", -1));
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

  void testGetDescriptorsWithoutAliases() {
    auto &algFactory = AlgorithmFactory::Instance();
    std::vector<AlgorithmDescriptor> descriptors;
    descriptors = algFactory.getDescriptors(false, false);
    const auto sizeDescExcludeBefore = descriptors.size();
    algFactory.subscribe<ToyAlgorithm>();
    descriptors = algFactory.getDescriptors(false, false);
    const auto sizeDescExcludeAfter = descriptors.size();

    auto resAlg = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("ToyAlgorithm" == d.name) && ("Dog" == d.alias) && (1 == d.version));
    });

    auto resAlias = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("Dog" == d.name) && ("" == d.alias) && (1 == d.version));
    });

    TS_ASSERT_EQUALS(sizeDescExcludeAfter - sizeDescExcludeBefore, 1);
    TS_ASSERT(resAlg != descriptors.cend());
    TS_ASSERT(resAlias == descriptors.cend());

    algFactory.unsubscribe("ToyAlgorithm", 1);

    descriptors = algFactory.getDescriptors(true, false);
    const auto sizeDescIncludeBefore = descriptors.size();
    algFactory.subscribe<ToyAlgorithm>();
    descriptors = algFactory.getDescriptors(true, false);
    const auto sizeDescIncludeAfter = descriptors.size();

    resAlg = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("ToyAlgorithm" == d.name) && ("Dog" == d.alias) && (1 == d.version));
    });

    resAlias = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("Dog" == d.name) && ("" == d.alias) && (1 == d.version));
    });

    TS_ASSERT_EQUALS(sizeDescIncludeAfter - sizeDescIncludeBefore, 1);
    TS_ASSERT(resAlg != descriptors.cend());
    TS_ASSERT(resAlias == descriptors.cend());

    algFactory.unsubscribe("ToyAlgorithm", 1);
  }

  void testGetDescriptorsWithAliases() {
    auto &algFactory = AlgorithmFactory::Instance();
    std::vector<AlgorithmDescriptor> descriptors;
    descriptors = algFactory.getDescriptors(false, true);
    const auto sizeDescExcludeBefore = descriptors.size();
    algFactory.subscribe<ToyAlgorithm>();
    descriptors = algFactory.getDescriptors(false, true);
    const auto sizeDescExcludeAfter = descriptors.size();

    auto resAlg = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("ToyAlgorithm" == d.name) && ("Dog" == d.alias) && (1 == d.version));
    });

    auto resAlias = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("Dog" == d.name) && ("" == d.alias) && (1 == d.version));
    });

    TS_ASSERT_EQUALS(sizeDescExcludeAfter - sizeDescExcludeBefore, 2);
    TS_ASSERT(resAlg != descriptors.cend());
    TS_ASSERT(resAlias != descriptors.cend());

    algFactory.unsubscribe("ToyAlgorithm", 1);

    descriptors = algFactory.getDescriptors(true, true);
    const auto sizeDescIncludeBefore = descriptors.size();
    algFactory.subscribe<ToyAlgorithm>();
    descriptors = algFactory.getDescriptors(true, true);
    const auto sizeDescIncludeAfter = descriptors.size();

    resAlg = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("ToyAlgorithm" == d.name) && ("Dog" == d.alias) && (1 == d.version));
    });

    resAlias = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Cat" == d.category) && ("Dog" == d.name) && ("" == d.alias) && (1 == d.version));
    });

    TS_ASSERT_EQUALS(sizeDescIncludeAfter - sizeDescIncludeBefore, 2);
    TS_ASSERT(resAlg != descriptors.cend());
    TS_ASSERT(resAlias != descriptors.cend());

    algFactory.unsubscribe("ToyAlgorithm", 1);
  }

  void testLowerCaseAlliasesAreNotAddedToDescriptions() {
    auto &algFactory = AlgorithmFactory::Instance();
    algFactory.subscribe<LowerCaseAliasAlgorithm>();
    std::vector<AlgorithmDescriptor> descriptors;
    descriptors = algFactory.getDescriptors(false, true);

    auto resAlg = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("Lower" == d.name) && ("lower" == d.alias) && (1 == d.version));
    });

    auto resAlias = std::find_if(descriptors.cbegin(), descriptors.cend(), [](const AlgorithmDescriptor &d) {
      return (("lower" == d.name) && ("" == d.alias) && (1 == d.version));
    });

    TS_ASSERT(resAlg != descriptors.cend());
    TS_ASSERT(resAlias == descriptors.cend());

    algFactory.unsubscribe("LowerCaseAliasAlgorithm", 1);
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
    TS_ASSERT_THROWS_NOTHING(validCategories = algFactory.getCategoriesWithState());
    size_t noOfCats = validCategories.size();
    TS_ASSERT_DIFFERS(validCategories.find("Fake"), validCategories.end());

    algFactory.unsubscribe("CategoryAlgorithm", 1);
    TS_ASSERT_THROWS_NOTHING(validCategories = algFactory.getCategoriesWithState());
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

  void testAliasPointsToCorrectVersion() {
    auto &algFactory = AlgorithmFactory::Instance();
    algFactory.subscribe<CoolAlgorithm1>();
    algFactory.subscribe<CoolAlgorithm2>();

    const auto nameAndVersion = algFactory.getRealNameFromAlias("TheCoolestAlgorithm");

    TS_ASSERT_EQUALS(nameAndVersion->first, "CoolAlgorithm");
    TS_ASSERT_EQUALS(nameAndVersion->second, 1);
  }
};
