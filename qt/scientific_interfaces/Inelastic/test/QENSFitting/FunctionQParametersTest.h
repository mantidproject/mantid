// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "QENSFitting/FunctionQParameters.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace testing;

namespace {

std::vector<std::string> getTextAxisLabels(std::string const &labelTypes) {
  if (labelTypes == "Width") {
    return {"f0.Width", "f1.Width", "f2.Width"};
  } else if (labelTypes == "EISF") {
    return {"f0.EISF", "f1.EISF", "f2.EISF"};
  } else if (labelTypes == "All") {
    return {"f0.Width", "f1.Width", "f2.Width", "f0.EISF", "f1.EISF", "f2.EISF"};
  } else {
    return {"f0.UnsupportedParameter"};
  }
}

} // namespace

class FunctionQParametersTest : public CxxTest::TestSuite {
public:
  static FunctionQParametersTest *createSuite() { return new FunctionQParametersTest(); }

  static void destroySuite(FunctionQParametersTest *suite) { delete suite; }

  void test_names_returns_empty_vector_when_width_labels_do_not_exist() {
    std::vector<std::string> const expectedNames{};

    auto const parameters = createFunctionQParameters("EISF");
    TS_ASSERT_EQUALS(expectedNames, parameters->names("Width"));
  }

  void test_names_returns_empty_vector_when_eisf_labels_do_not_exist() {
    std::vector<std::string> const expectedNames{};

    auto const parameters = createFunctionQParameters("Width");
    TS_ASSERT_EQUALS(expectedNames, parameters->names("EISF"));
  }

  void test_spectra_throws_expection_when_invalid_label_type_provided() {
    auto const parameters = createFunctionQParameters("All");
    TS_ASSERT_THROWS(parameters->spectra("UnknownParameter"), std::logic_error const &);
  }

  void test_types_returns_only_width_when_expected() {
    std::vector<std::string> const expectedTypes{"Width"};

    auto const parameters = createFunctionQParameters("Width");
    TS_ASSERT_EQUALS(expectedTypes, parameters->types());
  }

  void test_types_returns_only_eisf_when_expected() {
    std::vector<std::string> const expectedTypes{"EISF"};

    auto const parameters = createFunctionQParameters("EISF");
    TS_ASSERT_EQUALS(expectedTypes, parameters->types());
  }

  void test_types_returns_empty_vector_when_expected() {
    std::vector<std::string> const expectedTypes{};

    auto const parameters = createFunctionQParameters("None");
    TS_ASSERT_EQUALS(expectedTypes, parameters->types());
  }

  void test_names_returns_the_expected_width_parameter_names() {
    std::vector<std::string> const expectedNames{"f0.Width", "f1.Width", "f2.Width"};

    auto const parameters = createFunctionQParameters("All");
    TS_ASSERT_EQUALS(expectedNames, parameters->names("Width"));
  }

  void test_names_returns_the_expected_eisf_parameter_names() {
    std::vector<std::string> const expectedNames{"f0.EISF", "f1.EISF", "f2.EISF"};

    auto const parameters = createFunctionQParameters("All");
    TS_ASSERT_EQUALS(expectedNames, parameters->names("EISF"));
  }

  void test_spectra_returns_the_expected_width_parameter_spectra() {
    std::vector<std::size_t> const expectedSpectra{0u, 1u, 2u};

    auto const parameters = createFunctionQParameters("All");
    TS_ASSERT_EQUALS(expectedSpectra, parameters->spectra("Width"));
  }

  void test_spectra_returns_the_expected_eisf_parameter_spectra() {
    std::vector<std::size_t> const expectedSpectra{3u, 4u, 5u};

    auto const parameters = createFunctionQParameters("All");
    TS_ASSERT_EQUALS(expectedSpectra, parameters->spectra("EISF"));
  }

  void test_types_returns_both_types_when_expected() {
    std::vector<std::string> const expectedTypes{"Width", "EISF"};

    auto const parameters = createFunctionQParameters("All");
    TS_ASSERT_EQUALS(expectedTypes, parameters->types());
  }

private:
  std::unique_ptr<FunctionQParameters> createFunctionQParameters(std::string const &labelTypes) {
    auto const labels = getTextAxisLabels(labelTypes);
    return std::make_unique<FunctionQParameters>(createWorkspaceWithTextAxis(static_cast<int>(labels.size()), labels));
  }
};
