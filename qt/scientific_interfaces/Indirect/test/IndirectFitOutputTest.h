// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDIRECTFITOUTPUTTEST_H_
#define MANTID_INDIRECTFITOUTPUTTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitOutput.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

namespace {

MatrixWorkspace_sptr
createPopulatedworkspace(std::vector<double> const &xValues,
                         std::vector<double> const &yValues,
                         int const numberOfSpectra,
                         std::vector<std::string> const &verticalAxisNames) {
  auto createWorkspaceAlgorithm =
      AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
  createWorkspaceAlgorithm->initialize();
  createWorkspaceAlgorithm->setChild(true);
  createWorkspaceAlgorithm->setLogging(false);
  createWorkspaceAlgorithm->setProperty("DataX", xValues);
  createWorkspaceAlgorithm->setProperty("DataY", yValues);
  createWorkspaceAlgorithm->setProperty("NSpec", numberOfSpectra);
  createWorkspaceAlgorithm->setProperty("VerticalAxisUnit", "Text");
  createWorkspaceAlgorithm->setProperty("VerticalAxisValues",
                                        verticalAxisNames);
  createWorkspaceAlgorithm->setProperty("OutputWorkspace", "OutputResults");
  createWorkspaceAlgorithm->execute();
  return createWorkspaceAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr createPopulatedworkspace(int const &numberOfSpectra) {
  std::vector<double> xValues{1.0, 2.0, 3.0, 4.0, 5.0};
  std::vector<double> yValues{1.0, 2.0, 3.0, 4.0, 5.0};
  std::vector<std::string> const verticalAxisNames{
      "Height", "Height_Err", "Msd", "Msd_Err", "Chi_squared"};
  return createPopulatedworkspace(xValues, yValues, numberOfSpectra,
                                  verticalAxisNames);
}

IndirectFitData getIndirectFitData(int const &numberOfSpectra) {
  auto const workspace = createWorkspace(numberOfSpectra);
  Spectra const spec =
      Spectra(IDAWorkspaceIndex{0},
              IDAWorkspaceIndex::cast(workspace->getNumberHistograms() - 1));
  IndirectFitData data(workspace, spec);
  return data;
}

ITableWorkspace_sptr getEmptyTableWorkspace() {
  auto table = WorkspaceFactory::Instance().createTable();
  std::vector<std::string> columnHeadings{"Height", "Height_Err", "Msd",
                                          "Msd_Err", "Chi_squared"};
  for (auto i = 0u; i < columnHeadings.size(); ++i)
    table->addColumn("double", columnHeadings[i]);
  return table;
}

ITableWorkspace_sptr getPopulatedTable(std::size_t const &size) {
  auto table = getEmptyTableWorkspace();
  for (auto i = 0u; i < size; ++i) {
    TableRow row = table->appendRow();
    row << 14.675 << 0.047 << 0.001 << 0.514 << 0.0149;
  }
  return table;
}

WorkspaceGroup_sptr getPopulatedGroup(std::size_t const &size) {
  WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
  for (auto i = 0u; i < size; ++i)
    group->addWorkspace(createPopulatedworkspace(5));
  return group;
}

std::unique_ptr<IndirectFitOutput>
createFitOutput(WorkspaceGroup_sptr resultGroup,
                ITableWorkspace_sptr parameterTable,
                WorkspaceGroup_sptr resultWorkspace, IndirectFitData *fitData,
                std::size_t spectrum) {
  return std::make_unique<IndirectFitOutput>(resultGroup, parameterTable,
                                             resultWorkspace, fitData,
                                             IDAWorkspaceIndex::cast(spectrum));
}

std::unordered_map<std::string, std::string>
getNewParameterNames(std::vector<std::string> const &currentNames) {
  std::unordered_map<std::string, std::string> newParameterNames;
  newParameterNames[currentNames[0]] = "Width_Err";
  newParameterNames[currentNames[1]] = "MSD_Err";
  return newParameterNames;
}

} // namespace

class IndirectFitOutputTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFitOutputTest() { FrameworkManager::Instance(); }

  static IndirectFitOutputTest *createSuite() {
    return new IndirectFitOutputTest();
  }

  static void destroySuite(IndirectFitOutputTest *suite) { delete suite; }

  void setUp() override {
    m_resultGroup = getPopulatedGroup(2);
    m_workspacesGroup = getPopulatedGroup(2);
    m_parameterTable = getPopulatedTable(2);

    m_fitData = std::make_unique<IndirectFitData>(getIndirectFitData(5));

    storeWorkspacesInADS(m_resultGroup, m_workspacesGroup, m_parameterTable);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_resultGroup.reset();
    m_workspacesGroup.reset();
    m_parameterTable.reset();
  }

  void
  test_that_IndirectFitOutput_constructor_will_set_the_values_of_the_output_data() {
    auto const output = getFitOutputData();

    TS_ASSERT(output->getLastResultGroup());
    TS_ASSERT(output->getLastResultWorkspace());
    TS_ASSERT_EQUALS(output->getLastResultGroup()->getNumberOfEntries(), 2);
    TS_ASSERT_EQUALS(output->getLastResultWorkspace()->getNumberOfEntries(), 2);
    TS_ASSERT_EQUALS(output->getResultParameterNames().size(), 5);
  }

  void
  test_that_the_group_workspaces_stored_are_equal_to_the_workspaces_inputed() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    TS_ASSERT_EQUALS(output->getLastResultGroup(), m_workspacesGroup);
    TS_ASSERT_EQUALS(output->getLastResultWorkspace(), m_resultGroup);
  }

  void
  test_that_isSpectrumFit_returns_false_if_the_spectrum_has_not_been_previously_fit() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    TS_ASSERT(!output->isSpectrumFit(m_fitData.get(), IDAWorkspaceIndex{7}));
  }

  void
  test_that_isSpectrumFit_returns_true_if_the_spectrum_has_been_previously_fit() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    TS_ASSERT(output->isSpectrumFit(m_fitData.get(), IDAWorkspaceIndex{0}));
  }

  void
  test_that_getParameters_returns_an_empty_map_when_the_spectrum_number_provided_is_out_of_range() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    TS_ASSERT(
        output->getParameters(m_fitData.get(), IDAWorkspaceIndex{7}).empty());
  }

  void
  test_that_getParameters_returns_the_correct_parameter_values_when_the_spectrum_number_and_IndirectFitData_provided_is_valid() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    auto const parameters =
        output->getParameters(m_fitData.get(), IDAWorkspaceIndex{0});
    TS_ASSERT_EQUALS(parameters.size(), 2);
    TS_ASSERT_EQUALS(parameters.at("Height_Err").value, 0.047);
    TS_ASSERT_EQUALS(parameters.at("Msd_Err").value, 0.514);
  }

  void
  test_that_getResultLocation_returns_none_when_the_spectrum_number_provided_is_out_of_range() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    TS_ASSERT(
        !output->getResultLocation(m_fitData.get(), IDAWorkspaceIndex{7}));
  }

  void
  test_that_getResultLocation_returns_the_ResultLocation_when_the_spectrum_number_and_IndirectFitData_provided_is_valid() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    auto const resultLocation =
        output->getResultLocation(m_fitData.get(), IDAWorkspaceIndex{0});
    TS_ASSERT(resultLocation);
    TS_ASSERT_EQUALS(resultLocation->result.lock(), m_workspacesGroup);
  }

  void
  test_that_getResultParameterNames_gets_the_parameter_names_which_were_provided_as_input_data() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);
    std::vector<std::string> const expectedParameters{
        "Height", "Height_Err", "Msd", "Msd_Err", "Chi_squared"};

    auto const parameters = output->getResultParameterNames();

    TS_ASSERT_EQUALS(parameters.size(), 5);
    for (auto i = 0u; i < parameters.size(); ++i)
      TS_ASSERT_EQUALS(parameters[i], expectedParameters[i]);
  }

  void
  test_that_mapParameterNames_will_remap_the_parameters_to_correspond_to_the_provided_parameter_names() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);
    auto const newParameterNames =
        getNewParameterNames({"Height_Err", "Msd_Err"});

    output->mapParameterNames(newParameterNames, m_fitData.get());

    auto const parameters =
        output->getParameters(m_fitData.get(), IDAWorkspaceIndex{0});
    TS_ASSERT_EQUALS(parameters.size(), 2);
    TS_ASSERT_EQUALS(parameters.at("Width_Err").value, 0.047);
    TS_ASSERT_EQUALS(parameters.at("MSD_Err").value, 0.514);
  }

  void
  test_that_mapParameterNames_will_not_remap_the_parameters_when_the_provided_old_parameter_names_do_not_exist() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);
    auto const newParameterNames = getNewParameterNames({"None1", "None2"});

    output->mapParameterNames(newParameterNames, m_fitData.get());

    auto const parameters =
        output->getParameters(m_fitData.get(), IDAWorkspaceIndex{0});
    TS_ASSERT(parameters.at("Height_Err").value);
    TS_ASSERT(parameters.at("Msd_Err").value);
  }

  void
  test_that_addOutput_will_add_new_fitData_without_overwriting_existing_data() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);
    auto const data2 = std::make_unique<IndirectFitData>(getIndirectFitData(2));

    output->addOutput(m_workspacesGroup, m_parameterTable, m_resultGroup,
                      data2.get(), IDAWorkspaceIndex{0});

    TS_ASSERT(
        !output->getParameters(m_fitData.get(), IDAWorkspaceIndex{0}).empty());
    TS_ASSERT(
        !output->getParameters(data2.get(), IDAWorkspaceIndex{0}).empty());
  }

  void test_that_removeOutput_will_erase_the_provided_fitData() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);

    output->removeOutput(m_fitData.get());

    TS_ASSERT(
        output->getParameters(m_fitData.get(), IDAWorkspaceIndex{0}).empty());
    TS_ASSERT(
        !output->getResultLocation(m_fitData.get(), IDAWorkspaceIndex{0}));
  }

  void test_that_removeOutput_will_not_delete_fitData_which_is_not_specified() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);
    auto const data2 = std::make_unique<IndirectFitData>(getIndirectFitData(2));

    output->addOutput(m_workspacesGroup, m_parameterTable, m_resultGroup,
                      data2.get(), IDAWorkspaceIndex{0});
    output->removeOutput(data2.get());

    TS_ASSERT(!output->getParameters(m_fitData.get(), IDAWorkspaceIndex{0}).empty());
    TS_ASSERT(output->getParameters(data2.get(), IDAWorkspaceIndex{0}).empty());
  }

  void
  test_that_removeOutput_does_not_throw_when_provided_fitData_which_does_not_exist() {
    auto const output = createFitOutput(m_workspacesGroup, m_parameterTable,
                                        m_resultGroup, m_fitData.get(), 0);
    auto const data2 = std::make_unique<IndirectFitData>(getIndirectFitData(2));

    TS_ASSERT_THROWS_NOTHING(output->removeOutput(data2.get()));
  }

  void
  test_that_the_resultworkspace_is_renamed_to_have_the_correct_name_after_a_fit_is_executed() {
    auto const resultGroup = getPopulatedGroup(1);
    m_ads->addOrReplace("ConvFit_1L_Results_1", resultGroup);

    (void)createFitOutput(m_workspacesGroup, m_parameterTable, resultGroup,
                          m_fitData.get(), 0);

    TS_ASSERT(m_ads->doesExist("ConvFit_1L_Result"));
  }

  // void
  // test_that_the_resultworkspace_is_renamed_to_have_the_correct_name_after_a_fit_is_executed_with_multiple_data()
  // {
  //  (void)getFitOutputData();
  //  TS_ASSERT(m_ads->doesExist("MultiConvFit_1L_Workspaces_1__s0_to_4_Result"));
  //}

private:
  /// This will return fit output with workspaces still stored in the ADS
  std::unique_ptr<IndirectFitOutput> getFitOutputData() {
    storeWorkspacesInADS(m_workspacesGroup, m_resultGroup, m_parameterTable);
    return createFitOutput(m_workspacesGroup, m_parameterTable, m_resultGroup,
                           m_fitData.get(), 0);
  }

  /// Store workspaces in ADS and won't destruct the ADS when leaving scope
  void storeWorkspacesInADS(WorkspaceGroup_sptr workspacesGroup,
                            WorkspaceGroup_sptr resultGroup,
                            ITableWorkspace_sptr table) {
    std::string const nameStart = resultGroup->size() > 1 ? "Multi" : "";
    m_ads = std::make_unique<SetUpADSWithWorkspace>(
        nameStart + "ConvFit_1L_Workspaces", workspacesGroup);
    m_ads->addOrReplace(nameStart + "ConvFit_1L_Results", resultGroup);
    m_ads->addOrReplace(nameStart + "ConvFit_1L_Parameters", table);
  }

  WorkspaceGroup_sptr m_resultGroup;
  WorkspaceGroup_sptr m_workspacesGroup;
  ITableWorkspace_sptr m_parameterTable;
  std::unique_ptr<IndirectFitData> m_fitData;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};

#endif // MANTID_INDIRECTFITOUTPUTTEST_H