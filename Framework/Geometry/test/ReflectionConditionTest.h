// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_REFLECTIONCONDITIONTEST_H_
#define MANTID_GEOMETRY_REFLECTIONCONDITIONTEST_H_

#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <unordered_set>

using namespace Mantid::Geometry;

class ReflectionConditionTest : public CxxTest::TestSuite {
public:
  void checkRC(ReflectionCondition &rc, int *h, int *k, int *l, int *valid,
               size_t count) {
    for (size_t i = 0; i < count; i++) {
      bool v = rc.isAllowed(h[i], k[i], l[i]);
      TS_ASSERT_EQUALS((valid[i] == 1), v);
    }
  }

  void test_ReflectionConditionCFaceCentred() {
    ReflectionConditionCFaceCentred rc;
    int h[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    int k[9] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
    int l[9] = {0, 1, 3, 4, 5, 6, 7, 8, 9};
    int v[9] = {1, 0, 1, 0, 1, 0, 1, 0, 1};
    checkRC(rc, h, k, l, v, 9);
  }

  void test_ReflectionConditionAllFaceCentred() {
    ReflectionConditionAllFaceCentred rc;
    int h[5] = {0, 1, 0, 1, 1};
    int k[5] = {0, 1, 0, 3, 2};
    int l[5] = {0, 1, 1, 1, 3};
    int v[5] = {1, 1, 0, 1, 0};
    checkRC(rc, h, k, l, v, 5);
  }

  void test_getAllReflectionConditions() {
    std::vector<ReflectionCondition_sptr> refs = getAllReflectionConditions();
    TS_ASSERT_EQUALS(refs.size(), 9);
    TS_ASSERT(refs[0]);
    TS_ASSERT_EQUALS(refs[0]->getName(), "Primitive");
    TS_ASSERT(refs[8]);
  }

  void test_ReflectionConditionSymbols() {
    std::unordered_set<std::string> centeringSymbols;
    centeringSymbols.insert("P");
    centeringSymbols.insert("A");
    centeringSymbols.insert("B");
    centeringSymbols.insert("C");
    centeringSymbols.insert("F");
    centeringSymbols.insert("I");
    centeringSymbols.insert("Robv");
    centeringSymbols.insert("Rrev");
    centeringSymbols.insert("H");

    std::vector<ReflectionCondition_sptr> refs = getAllReflectionConditions();
    for (auto &ref : refs) {
      TSM_ASSERT_DIFFERS(ref->getSymbol(),
                         centeringSymbols.find(ref->getSymbol()),
                         centeringSymbols.end());
      centeringSymbols.erase(ref->getSymbol());
    }

    // All centering symbols are present if the set is empty.
    TS_ASSERT_EQUALS(centeringSymbols.size(), 0);
  }

  void test_getReflectionConditionNames() {
    auto conditions = getAllReflectionConditions();
    auto names = getAllReflectionConditionNames();

    TS_ASSERT_EQUALS(conditions.size(), names.size());

    // there should not be any duplicates in the names
    std::unordered_set<std::string> nameSet(names.begin(), names.end());

    TS_ASSERT_EQUALS(nameSet.size(), names.size())
  }

  void test_getReflectionConditionSymbols() {
    auto conditions = getAllReflectionConditions();
    auto symbols = getAllReflectionConditionSymbols();

    TS_ASSERT_EQUALS(conditions.size(), symbols.size());

    // there should not be any duplicates in the names
    std::unordered_set<std::string> symbolSet(symbols.begin(), symbols.end());

    TS_ASSERT_EQUALS(symbolSet.size(), symbols.size())
  }

  void test_getReflectionConditionByName() {
    auto names = getAllReflectionConditionNames();

    for (auto name : names) {
      TSM_ASSERT_THROWS_NOTHING("Problem with ReflectionCondition: " + name,
                                getReflectionConditionByName(name));
    }

    TS_ASSERT_THROWS(getReflectionConditionByName("invalid"),
                     const std::invalid_argument &);
  }

  void test_getReflectionConditionBySymbol() {
    auto symbols = getAllReflectionConditionSymbols();

    for (auto symbol : symbols) {
      TSM_ASSERT_THROWS_NOTHING("Problem with ReflectionCondition: " + symbol,
                                getReflectionConditionBySymbol(symbol));
    }

    TS_ASSERT_THROWS(getReflectionConditionBySymbol("Q"),
                     const std::invalid_argument &);
  }
};

#endif /* MANTID_GEOMETRY_REFLECTIONCONDITIONTEST_H_ */
