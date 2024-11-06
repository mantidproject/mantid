// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
/// static Logger definition
Logger g_log("EnumeratedStringPropertyTest");
} // namespace
// namespace {
enum class CoolGuys : char { Fred, Joe, Bill, enum_count };
const std::vector<std::string> coolGuyNames{"Frederic", "Joseph", "William"};

// typedef EnumeratedStringProperty<CoolGuys, &coolGuyNames> COOLGUY_PROPERTY;
// typedef EnumeratedStringProperty<Cakes, &cakeNames> CAKE_PROPERTY;
//} // namespace

// 'Empty' algorithm class for tests
class testalg : public Algorithm {
public:
  testalg() : Algorithm() {}
  ~testalg() override = default;
  const std::string name() const override { return "testalg"; } ///< Algorithm's name for identification
  int version() const override { return 1; }                    ///< Algorithm's version for identification
  const std::string category() const override { return "Cat"; } ///< Algorithm's category for identification
  const std::string summary() const override { return "Test summary"; }

  void init() override {
    declareProperty(std::make_unique<EnumeratedStringProperty<CoolGuys, &coolGuyNames>>("testname"), "Test property");
  }
  void exec() override {}
};

class EnumeratedStringPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EnumeratedStringPropertyTest *createSuite() { return new EnumeratedStringPropertyTest(); }
  static void destroySuite(EnumeratedStringPropertyTest *suite) { delete suite; }

  void testAlgorithm() {
    g_log.notice("\ntestAlgorithm...");

    testalg alg;
    alg.initialize();
    alg.execute();
    TS_ASSERT_EQUALS(alg.existsProperty("testname"), true)
    TS_ASSERT_EQUALS(alg.getPropertyValue("testname"), "Frederic");

    TS_ASSERT_EQUALS(alg.getPropertyValue("testname"), coolGuyNames[0]);
    alg.setPropertyValue("testname", "Joseph");
    TS_ASSERT_EQUALS(alg.getPropertyValue("testname"), "Joseph");
  }

  void testAssign() {
    g_log.notice("\ntestAssign...");
    auto prop = EnumeratedStringProperty<CoolGuys, &coolGuyNames>("testname");
    // by default it is fine
    TS_ASSERT(prop.isValid().empty());
    TS_ASSERT_EQUALS(prop.value(), "Frederic");
    TS_ASSERT_EQUALS(prop(), CoolGuys::Fred);

    // set to a good value
    TS_ASSERT_THROWS_NOTHING(prop.setValue("Joseph"));
    TS_ASSERT(prop.isValid().empty());
    TS_ASSERT_EQUALS(prop.value(), "Joseph");
    TS_ASSERT_EQUALS(prop(), CoolGuys::Joe);

    // set to a bad value
    TS_ASSERT_THROWS_NOTHING(prop.setValue("Gauss"));
    TS_ASSERT(!prop.setValue("Gauss").empty());

    // set to an empty string
    TS_ASSERT_THROWS_NOTHING(prop.setValue(""));
    TS_ASSERT(!prop.setValue("").empty());
  }
};
