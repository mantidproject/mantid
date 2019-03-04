// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDIRECTFITDATACREATIONHELPERTEST_H_
#define MANTID_INDIRECTFITDATACREATIONHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;

namespace {

std::vector<std::string> getTextAxisLabels() {
  return {"f0.Width", "f1.Width", "f2.EISF"};
}

std::vector<double> getNumericAxisLabels() { return {2.2, 3.3, 4.4}; }

void storeWorkspaceInADS(std::string const &workspaceName,
                         MatrixWorkspace_sptr workspace) {
  SetUpADSWithWorkspace ads(workspaceName, workspace);
}

/// The classes TypeOne and TypeTwo are used to test AreSpectraEquals which
/// compares the values of boost::variant types
class TypeOne {
public:
  explicit TypeOne(const std::string &value) : m_value(value) {}

  const std::string &getValue() const { return m_value; }

  bool operator==(TypeOne const &value) const {
    return this->getValue() == value.getValue();
  }

private:
  std::string m_value;
};

class TypeTwo {
public:
  explicit TypeTwo(const std::size_t &value) : m_value(value) {}

  const std::size_t &getValue() const { return m_value; }

  bool operator==(TypeTwo const &value) const {
    return this->getValue() == value.getValue();
  }

private:
  std::size_t m_value;
};

using Types = boost::variant<TypeOne, TypeTwo>;

} // namespace

class IndirectFitDataCreationHelperTest : public CxxTest::TestSuite {
public:
  static IndirectFitDataCreationHelperTest *createSuite() {
    return new IndirectFitDataCreationHelperTest();
  }

  static void destroySuite(IndirectFitDataCreationHelperTest *suite) {
    delete suite;
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_the_constant_variables_have_the_values_expected() {
    TS_ASSERT_EQUALS(START_X_COLUMN, 2);
    TS_ASSERT_EQUALS(END_X_COLUMN, 3);
    TS_ASSERT_EQUALS(EXCLUDE_REGION_COLUMN, 4);
  }

  void
  test_that_createWorkspace_returns_a_workspace_with_the_number_of_spectra_specified() {
    auto const workspace = createWorkspace(10);
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 10);
  }

  void
  test_that_createInstrumentWorkspace_returns_a_workspace_with_the_number_of_spectra_specified() {
    auto const workspace = createInstrumentWorkspace(6, 5);
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 6);
  }

  void
  test_that_createWorkspaceWithTextAxis_returns_a_workspace_with_the_number_of_spectra_specified() {
    auto const workspace = createWorkspaceWithTextAxis(3, getTextAxisLabels());
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 3);
  }

  void
  test_that_createWorkspaceWithTextAxis_returns_a_workspace_with_the_text_axis_labels_specified() {
    auto const labels = getTextAxisLabels();
    auto const workspace = createWorkspaceWithTextAxis(3, labels);

    auto const yAxis = workspace->getAxis(1);

    for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
      TS_ASSERT_EQUALS(yAxis->label(index), labels[index]);
  }

  void
  test_that_createWorkspaceWithTextAxis_throws_when_the_number_of_spectra_is_not_equal_to_the_number_of_labels() {
    auto const labels = std::vector<std::string>({"f0.Width", "f1.EISF"});
    TS_ASSERT_THROWS(createWorkspaceWithTextAxis(6, labels),
                     std::runtime_error);
  }

  void
  test_that_createWorkspaceWithBinValues_returns_a_workspace_with_the_number_of_spectra_specified() {
    auto const workspace =
        createWorkspaceWithBinValues(3, getNumericAxisLabels(), 3);
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 3);
  }

  void
  test_that_createWorkspaceWithBinValues_throws_when_the_number_of_bins_is_not_equal_to_the_number_of_labels() {
    auto const labels = getNumericAxisLabels();
    TS_ASSERT_THROWS(createWorkspaceWithBinValues(3, labels, 2),
                     std::runtime_error);
  }

  void
  test_that_createWorkspaceWithBinValues_returns_a_workspace_with_the_numeric_bin_axis_labels_specified() {
    auto const labels = getNumericAxisLabels();
    auto const workspace =
        createWorkspaceWithBinValues(3, getNumericAxisLabels(), 3);

    auto const xAxis = workspace->getAxis(0);

    auto const expectedLabels = std::vector<std::string>{"2.2", "3.3", "4.4"};
    for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
      TS_ASSERT_EQUALS(xAxis->label(index), expectedLabels[index]);
  }

  void
  test_that_createGroupWorkspace_will_create_a_group_workspace_with_the_expected_number_of_entries() {
    auto const group = createGroupWorkspace(3, 5);

    TS_ASSERT(group->isGroup());
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 3);
  }

  void
  test_that_createGroupWorkspaceWithTextAxes_will_create_a_group_workspace_containing_workspace_with_the_specified_number_of_spectra() {
    auto const group =
        createGroupWorkspaceWithTextAxes(5, getTextAxisLabels(), 3);

    TS_ASSERT(group->isGroup());
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 5);
  }

  void
  test_that_setWorkspaceEFixed_does_not_throw_when_setting_EFixed_values() {
    auto workspace = createInstrumentWorkspace(6, 5);
    TS_ASSERT_THROWS_NOTHING(setWorkspaceEFixed(workspace, 6));
  }

  void test_that_setWorkspaceBinEdges_returns_a_workspace_with_binEdges_set() {
    auto const workspace =
        setWorkspaceBinEdges(createInstrumentWorkspace(6, 5), 6, 5);
    for (std::size_t i = 0; i < workspace->getNumberHistograms(); ++i)
      TS_ASSERT(workspace->binEdges(i));
  }

  void
  test_that_setWorkspaceBinEdges_throws_when_provided_an_out_of_range_number_of_spectra() {
    auto workspace = createInstrumentWorkspace(6, 5);
    TS_ASSERT_THROWS_ANYTHING(setWorkspaceBinEdges(workspace, 8, 5));
  }

  void
  test_that_SetUpADSWithWorkspace_returns_true_when_a_workspace_exists_in_the_ads() {
    auto const workspace = createWorkspace(10);

    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    TS_ASSERT(ads.doesExist("WorkspaceName"));
  }

  void
  test_that_SetUpADSWithWorkspace_returns_false_when_a_workspace_does_not_exists_in_the_ads() {
    auto const workspace = createWorkspace(10);

    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    TS_ASSERT(!ads.doesExist("WorkspaceWrongName"));
  }

  void
  test_that_SetUpADSWithWorkspace_retrieves_the_given_workspace_as_expected() {
    auto const workspace = createWorkspace(10);

    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    TS_ASSERT(ads.doesExist("WorkspaceName"));
    auto const storedWorkspace = ads.retrieveWorkspace("WorkspaceName");
    TS_ASSERT(storedWorkspace)
    TS_ASSERT_EQUALS(storedWorkspace->getNumberHistograms(), 10);
  }

  void
  test_that_SetUpADSWithWorkspace_is_instantiated_with_the_given_workspace_as_expected() {
    auto const workspace = createWorkspace(10);

    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    auto const storedWorkspace = ads.retrieveWorkspace("WorkspaceName");
    TS_ASSERT_EQUALS(storedWorkspace, workspace);
    TS_ASSERT_EQUALS(storedWorkspace->getNumberHistograms(), 10);
  }

  void
  test_that_SetUpADSWithWorkspace_adds_a_given_workspace_to_the_ads_as_expected() {
    auto const workspace = createWorkspace(10);

    SetUpADSWithWorkspace ads("WorkspaceName1", workspace);
    ads.addOrReplace("WorkspaceName2", workspace);

    TS_ASSERT(ads.doesExist("WorkspaceName1"));
    TS_ASSERT(ads.doesExist("WorkspaceName2"));
    auto const storedWorkspace = ads.retrieveWorkspace("WorkspaceName2");
    TS_ASSERT_EQUALS(storedWorkspace->getNumberHistograms(), 10);
  }

  void
  test_that_the_ads_instance_is_not_destructed_when_it_goes_out_of_scope() {
    auto const workspace = createWorkspace(10);

    storeWorkspaceInADS("WorkspaceName", workspace);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("WorkspaceName"));
  }

  void
  test_that_AreSpectraEqual_returns_true_when_given_two_identical_values_of_same_type() {
    Types const value1 = TypeOne("SameValue");
    Types const value2 = TypeOne("SameValue");
    TS_ASSERT(boost::apply_visitor(AreSpectraEqual(), value1, value2));
  }

  void
  test_that_AreSpectraEqual_returns_false_when_given_two_different_values_of_the_same_type() {
    Types const value1 = TypeOne("Value");
    Types const value2 = TypeOne("DifferentValue");

    TS_ASSERT(!boost::apply_visitor(AreSpectraEqual(), value1, value2));
  }

  void
  test_that_AreSpectraEqual_returns_false_when_given_two_different_values_with_different_types() {
    Types const value1 = TypeOne("Value");
    Types const value2 = TypeTwo(2);

    TS_ASSERT(!boost::apply_visitor(AreSpectraEqual(), value1, value2));
  }
};

#endif // MANTID_INDIRECTFITDATACREATIONHELPER_TEST
