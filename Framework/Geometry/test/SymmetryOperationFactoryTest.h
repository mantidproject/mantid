// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"

#include <boost/lexical_cast.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class SymmetryOperationFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryOperationFactoryTest *createSuite() {
    return new SymmetryOperationFactoryTest();
  }
  static void destroySuite(SymmetryOperationFactoryTest *suite) {
    delete suite;
  }

  SymmetryOperationFactoryTest() {
    SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
  }

  ~SymmetryOperationFactoryTest() override {
    SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
  }

  void testCreateSymOp() {
    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOp("fake2"),
                     Mantid::Kernel::Exception::ParseError);

    // createSymOp also works when an operation is not subscribed
    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
    TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"),
                     false);

    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));

    // it's automatically registered
    TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"),
                     true);
  }

  void testCreateSymOpsVector() {
    std::vector<std::string> opStrings{"x,y,z"};

    std::vector<SymmetryOperation> symOps =
        SymmetryOperationFactory::Instance().createSymOps(opStrings);
    TS_ASSERT_EQUALS(symOps.size(), 1);
    TS_ASSERT_EQUALS(symOps.front().identifier(), "x,y,z");

    // Add another one
    opStrings.emplace_back("-x,-y,-z");

    TS_ASSERT_THROWS_NOTHING(
        symOps = SymmetryOperationFactory::Instance().createSymOps(opStrings));
    TS_ASSERT_EQUALS(symOps.size(), 2);
    TS_ASSERT_EQUALS(symOps.front().identifier(), "x,y,z");
    TS_ASSERT_EQUALS(symOps.back().identifier(), "-x,-y,-z");

    opStrings.emplace_back("doesNotWork");
    TS_ASSERT_THROWS(
        symOps = SymmetryOperationFactory::Instance().createSymOps(opStrings),
        Mantid::Kernel::Exception::ParseError);
  }

  void testCreateSymOpsString() {
    std::string validOne("-x,-y,-z");
    std::string validTwo("-x,-y,-z; x+1/2,y+1/2,z+1/2");
    std::string validThree("-x,-y,-z; x+1/2,y+1/2,z+1/2; x,-y,z");

    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().createSymOps(validOne));
    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().createSymOps(validTwo));
    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().createSymOps(validThree));

    std::string invalidSep("-x,-y,-z | x+1/2,y+1/2,z+1/2");
    std::string invalidOne("-x,-y,-z; invalid");

    TS_ASSERT_THROWS(
        SymmetryOperationFactory::Instance().createSymOps(invalidSep),
        Mantid::Kernel::Exception::ParseError);
    TS_ASSERT_THROWS(
        SymmetryOperationFactory::Instance().createSymOps(invalidOne),
        Mantid::Kernel::Exception::ParseError);
  }

  void testUnsubscribe() {
    TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"),
                     true);

    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
    TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"),
                     false);

    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
  }

  void testIsSubscribed() {
    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
    TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"),
                     false);
    TS_ASSERT_THROWS_NOTHING(
        SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
    TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"),
                     true);
  }

  void testSubscribedSymbols() {
    // Clear factory
    std::vector<std::string> allSymbols =
        SymmetryOperationFactory::Instance().subscribedSymbols();
    for (auto &symbol : allSymbols) {
      SymmetryOperationFactory::Instance().unsubscribeSymOp(symbol);
    }

    // Subscribe two symmetry operations
    SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
    SymmetryOperationFactory::Instance().subscribeSymOp("-x,-y,-z");

    std::vector<std::string> symbols =
        SymmetryOperationFactory::Instance().subscribedSymbols();

    TS_ASSERT_EQUALS(symbols.size(), 2);
    TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "x,y,z"),
                      symbols.end());
    TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "-x,-y,-z"),
                      symbols.end());

    SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
    SymmetryOperationFactory::Instance().unsubscribeSymOp("-x,-y,-z");

    // Restore factory
    for (auto &symbol : allSymbols) {
      SymmetryOperationFactory::Instance().subscribeSymOp(symbol);
    }
  }
};

#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_ */
