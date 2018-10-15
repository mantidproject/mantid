#ifndef MANTID_INDIRECTFITOUTPUTTEST_H_
#define MANTID_INDIRECTFITOUTPUTTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitOutput.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

#include <iostream>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

IndirectFitData getIndirectFitData(int const &numberOfSpectra) {
  auto const workspace = createWorkspace(numberOfSpectra);
  Spectra const spec = std::make_pair(0u, workspace->getNumberHistograms() - 1);
  IndirectFitData data(workspace, spec);
  return data;
}

ITableWorkspace_sptr getEmptyTableWorkspace() {
  auto table = WorkspaceFactory::Instance().createTable();
  std::vector<std::string> columnHeadings{"axis-1", "Height",  "Height_Err",
                                          "Msd",    "Msd_Err", "Chi_squared"};
  for (auto i = 0u; i < columnHeadings.size(); ++i)
    table->addColumn("double", columnHeadings[i]);
  return table;
}

ITableWorkspace_sptr getPopulatedParameterTable(std::size_t const &size) {
  auto table = getEmptyTableWorkspace();
  for (auto i = 0u; i < size; ++i) {
    TableRow row = table->appendRow();
    row << 14.675 << 0.047 << 0.001 << 0.514 << 0.0149 << 5.138;
  }
  return table;
}

WorkspaceGroup_sptr
getGroupWorkspaceWithEmptyWorkspaces(std::size_t const &size) {
  WorkspaceGroup_sptr resultWorkspaces = boost::make_shared<WorkspaceGroup>();
  for (auto i = 0u; i < size; ++i)
    resultWorkspaces->addWorkspace(createWorkspace(10));
  return resultWorkspaces;
}

WorkspaceGroup_sptr getResultWorkspaces(std::size_t const &size) {
  return getGroupWorkspaceWithEmptyWorkspaces(size);
}

WorkspaceGroup_sptr getResultGroup(std::size_t const &size) {
  return getGroupWorkspaceWithEmptyWorkspaces(size);
}

// IndirectFitOutput createFitOutput(WorkspaceGroup_sptr resultGroup,
//                                  ITableWorkspace_sptr parameterTable,
//                                  WorkspaceGroup_sptr resultWorkspace,
//                                  IndirectFitData *fitData,
//                                  std::size_t &spectrum) {
//  return MantidQt::CustomInterfaces::IDA::IndirectFitOutput(
//      resultGroup, parameterTable, resultWorkspace, fitData, spectrum);
//}

} // namespace

class IndirectFitOutputTest : public CxxTest::TestSuite {
public:
  static IndirectFitOutputTest *createSuite() {
    return new IndirectFitOutputTest();
  }

  static void destroySuite(IndirectFitOutputTest *suite) { delete suite; }

  void
  test_that_IndirectFitOutput_constructor_will_set_the_values_of_the_output_data() {
    auto resultGroup = getResultGroup(2);
    auto resultWorkspaces = getResultWorkspaces(2);
    auto parameterTable = getPopulatedParameterTable(2);
    IndirectFitData *data = new IndirectFitData(getIndirectFitData(5));
    std::size_t spectrum = 0;

    IndirectFitOutput outputData(resultGroup, parameterTable, resultWorkspaces,
                                 data, spectrum);

    // auto const fitOutput = createFitOutput(resultGroup, parameterTable,
    //                                       resultWorkspaces, data, spectrum);
  }

  void
  test_that_isSpectrumFit_returns_false_if_the_spectrum_has_not_been_previously_fit() {
  }

  void
  test_that_isSpectrumFit_returns_true_if_the_spectrum_has_been_previously_fit() {
  }

  void test_test() { std::cout << "Hello"; }
};

#endif
