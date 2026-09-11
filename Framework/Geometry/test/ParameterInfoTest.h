// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
// makeWrappers() returns a unique_ptr<DetectorInfo>, so the complete type is needed here.
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/Instrument/ParameterInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

#include <memory>
#include <sstream>

using namespace Mantid::Geometry;

class ParameterInfoTest : public CxxTest::TestSuite {
public:
  static ParameterInfoTest *createSuite() { return new ParameterInfoTest(); }
  static void destroySuite(ParameterInfoTest *suite) { delete suite; }

  void test_get_returns_null_for_absent_component_and_absent_name() {
    ParameterInfo parameters;
    TS_ASSERT(!parameters.get(0, "anything"));
    parameters.add(0, makeDouble("present", 1.0));
    TS_ASSERT(!parameters.get(0, "absent"));
    TS_ASSERT(!parameters.get(1, "present"));
  }

  void test_add_and_get_roundtrip() {
    ParameterInfo parameters;
    parameters.add(3, makeDouble("height", 1.5));
    auto const parameter = parameters.get(3, "height");
    TS_ASSERT(parameter);
    TS_ASSERT_EQUALS(parameter->value<double>(), 1.5);
    TS_ASSERT(parameters.contains(3, "height"));
    TS_ASSERT(parameters.contains(3, "height", "double"));
    TS_ASSERT(!parameters.contains(3, "height", "string"));
  }

  /// Legacy ParameterMap matches parameter names with strcasecmp throughout, so the store must
  /// too -- a case-sensitive comparator would silently break IDFs that rely on this.
  void test_name_lookup_is_case_insensitive() {
    ParameterInfo parameters;
    parameters.add(0, makeDouble("Efixed", 25.0));

    TS_ASSERT(parameters.contains(0, "Efixed"));
    TS_ASSERT(parameters.contains(0, "efixed"));
    TS_ASSERT(parameters.contains(0, "EFIXED"));
    TS_ASSERT_EQUALS(parameters.get(0, "eFiXeD")->value<double>(), 25.0);
  }

  /// add() is add-or-replace, and its deduplication is case-insensitive for the same reason.
  void test_add_replaces_an_existing_parameter_regardless_of_case() {
    ParameterInfo parameters;
    parameters.add(0, makeDouble("Efixed", 25.0));
    parameters.add(0, makeDouble("efixed", 50.0));

    TS_ASSERT_EQUALS(parameters.size(), 1);
    TS_ASSERT_EQUALS(parameters.get(0, "Efixed")->value<double>(), 50.0);
  }

  /// The invariant a non-multi map would break: two functions on one component may each declare
  /// a parameter of the same short name, and both must survive.
  void test_two_fitting_parameters_sharing_a_name_both_survive() {
    ParameterInfo parameters;
    parameters.addFittingParameter(0, makeFitParameter("Alpha0", "IkedaCarpenterPV", 1.0), "IkedaCarpenterPV");
    parameters.addFittingParameter(0, makeFitParameter("Alpha0", "IkedaCarpenterMD", 2.0), "IkedaCarpenterMD");

    TS_ASSERT_EQUALS(parameters.size(), 2);
    TS_ASSERT_EQUALS(parameters.parameters(0).count("Alpha0"), 2);
    // names() deduplicates, matching legacy ParameterMap::names().
    TS_ASSERT_EQUALS(parameters.names(0).size(), 1);
  }

  void test_addFittingParameter_replaces_only_the_matching_function() {
    ParameterInfo parameters;
    parameters.addFittingParameter(0, makeFitParameter("Alpha0", "IkedaCarpenterPV", 1.0), "IkedaCarpenterPV");
    parameters.addFittingParameter(0, makeFitParameter("Alpha0", "IkedaCarpenterMD", 2.0), "IkedaCarpenterMD");
    parameters.addFittingParameter(0, makeFitParameter("Alpha0", "IkedaCarpenterPV", 9.0), "IkedaCarpenterPV");

    TS_ASSERT_EQUALS(parameters.size(), 2);
  }

  /// insert() is the rekeying path and must NOT deduplicate, or it would collapse the pair above.
  void test_insert_does_not_deduplicate() {
    ParameterInfo parameters;
    parameters.insert(0, makeDouble("thing", 1.0));
    parameters.insert(0, makeDouble("thing", 2.0));
    TS_ASSERT_EQUALS(parameters.size(), 2);
  }

  void test_parameters_returns_an_empty_container_for_an_unknown_component() {
    ParameterInfo const parameters;
    TS_ASSERT(parameters.parameters(42).empty());
  }

  void test_parameters_are_ordered_by_name_case_insensitively() {
    ParameterInfo parameters;
    parameters.add(0, makeDouble("zebra", 1.0));
    parameters.add(0, makeDouble("Apple", 2.0));
    parameters.add(0, makeDouble("mango", 3.0));

    std::vector<std::string> names;
    for (auto const &[name, parameter] : parameters.parameters(0)) {
      static_cast<void>(parameter);
      names.emplace_back(name);
    }
    std::vector<std::string> const expected{"Apple", "mango", "zebra"};
    TS_ASSERT_EQUALS(names, expected);
  }

  void test_getByType_matches_type_case_insensitively() {
    ParameterInfo parameters;
    parameters.add(0, makeDouble("height", 1.0));
    TS_ASSERT(parameters.getByType(0, "double"));
    TS_ASSERT(parameters.getByType(0, "DOUBLE"));
    TS_ASSERT(!parameters.getByType(0, "string"));
  }

  void test_clearParametersByName() {
    ParameterInfo parameters;
    parameters.add(0, makeDouble("keep", 1.0));
    parameters.add(0, makeDouble("drop", 2.0));
    parameters.add(1, makeDouble("drop", 3.0));

    parameters.clearParametersByName("drop");
    TS_ASSERT_EQUALS(parameters.size(), 1);
    TS_ASSERT(parameters.contains(0, "keep"));
    // The component that ended up empty is removed, keeping the store sparse.
    TS_ASSERT(parameters.parameters(1).empty());
  }

  void test_clearParametersByName_for_one_component_only() {
    ParameterInfo parameters;
    parameters.add(0, makeDouble("shared", 1.0));
    parameters.add(1, makeDouble("shared", 2.0));

    parameters.clearParametersByName(0, "shared");
    TS_ASSERT(!parameters.contains(0, "shared"));
    TS_ASSERT(parameters.contains(1, "shared"));
  }

  /// Recursion walks ComponentInfo::parent(), so it needs a real hierarchy.
  void test_getRecursive_walks_up_the_component_hierarchy() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 4);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    auto const &componentInfo = *std::get<0>(wrappers);

    size_t const bankIndex = componentInfo.root() - 3;
    size_t const detectorIndex = componentInfo.children(componentInfo.children(bankIndex)[0])[0];

    ParameterInfo parameters;
    parameters.add(bankIndex, makeDouble("bank_level", 7.0));

    TS_ASSERT(!parameters.get(detectorIndex, "bank_level"));
    auto const found = parameters.getRecursive(componentInfo, detectorIndex, "bank_level");
    TS_ASSERT(found);
    TS_ASSERT_EQUALS(found->value<double>(), 7.0);

    // A name nowhere in the ancestry terminates at the root rather than looping.
    TS_ASSERT(!parameters.getRecursive(componentInfo, detectorIndex, "nowhere"));
  }

  void test_getRecursive_prefers_the_nearest_ancestor() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 4);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    auto const &componentInfo = *std::get<0>(wrappers);

    size_t const bankIndex = componentInfo.root() - 3;
    size_t const detectorIndex = componentInfo.children(componentInfo.children(bankIndex)[0])[0];

    ParameterInfo parameters;
    parameters.add(componentInfo.root(), makeDouble("shadowed", 1.0));
    parameters.add(bankIndex, makeDouble("shadowed", 2.0));

    TS_ASSERT_EQUALS(parameters.getRecursive(componentInfo, detectorIndex, "shadowed")->value<double>(), 2.0);
  }

  void test_getRecursiveFittingParameter_disambiguates_by_function() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 4);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument);
    auto const &componentInfo = *std::get<0>(wrappers);
    size_t const bankIndex = componentInfo.root() - 3;

    ParameterInfo parameters;
    parameters.addFittingParameter(bankIndex, makeFitParameter("Alpha0", "IkedaCarpenterPV", 1.0), "IkedaCarpenterPV");
    parameters.addFittingParameter(bankIndex, makeFitParameter("Alpha0", "IkedaCarpenterMD", 2.0), "IkedaCarpenterMD");

    auto const md = parameters.getRecursiveFittingParameter(componentInfo, bankIndex, "Alpha0", "IkedaCarpenterMD");
    TS_ASSERT(md);
    TS_ASSERT_EQUALS(md->value<FitParameter>().getFunction(), "IkedaCarpenterMD");

    TS_ASSERT(!parameters.getRecursiveFittingParameter(componentInfo, bankIndex, "Alpha0", "NoSuchFunction"));
  }

  /// The rekeying performed by InstrumentVisitor must carry every parameter across, keyed by
  /// the matching component index.
  void test_rekeying_from_a_parameter_map_preserves_parameters() {
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular2(1, 4);
    auto bank = instrument->getComponentByName("bank1");
    ParameterMap pmap;
    pmap.addDouble(bank.get(), "my_double", 42.0);
    pmap.addString(bank.get(), "my_string", "hello");

    auto wrappers = InstrumentVisitor::makeWrappers(*instrument, &pmap);
    auto const &componentInfo = *std::get<0>(wrappers);
    size_t const bankIndex = componentInfo.indexOf(bank->getComponentID());

    TS_ASSERT_EQUALS(componentInfo.getNumberParameter(bankIndex, "my_double").at(0), 42.0);
    TS_ASSERT_EQUALS(componentInfo.getStringParameter(bankIndex, "my_string").at(0), "hello");
  }

private:
  static std::shared_ptr<Parameter> makeDouble(std::string const &name, double const value) {
    auto parameter = ParameterFactory::create("double", name);
    parameter->fromString(std::to_string(value));
    return parameter;
  }

  static std::shared_ptr<Parameter> makeFitParameter(std::string const &name, std::string const &function,
                                                     double const value) {
    auto parameter = ParameterFactory::create("fitting", name);
    // FitParameter's string form is a comma separated "value, function, name" followed by
    // optional entries; see operator>>(std::istream &, FitParameter &).
    std::ostringstream serialised;
    serialised << value << " , " << function << " , " << name;
    parameter->fromString(serialised.str());
    return parameter;
  }
};
