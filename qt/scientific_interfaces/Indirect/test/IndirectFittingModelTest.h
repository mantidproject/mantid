#ifndef MANTID_INDIRECTFITTINGMODELTEST_H_
#define MANTID_INDIRECTFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitData.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <iostream>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

// IndirectFitData getIndirectFitData(std::size_t const &numberOfSpectra,
//                                   std::size_t const &numberOfBins) {
//  auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(
//      numberOfSpectra, numberOfBins);
//  Spectra const spec = std::make_pair(0u, workspace->getNumberHistograms() -
//  1); IndirectFitData data(workspace, spec); return data;
//}

/// Simple class to set up the ADS with the configuration required
struct SetUpADSWithWorkspace {

  SetUpADSWithWorkspace(std::string const &inputWSName,
                        Workspace2D_sptr const &workspace) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, workspace);
  };

  ~SetUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
};

/// This is used to compare Spectra which is implemented as a boost::variant
struct AreSpectraEqual : public boost::static_visitor<bool> {

  template <typename T, typename U>
  bool operator()(const T &, const U &) const {
    return false; // cannot compare different types
  }

  template <typename T> bool operator()(const T &lhs, const T &rhs) const {
    return lhs == rhs;
  }
};

class DummyModel : public IndirectFittingModel {
public:
  ~DummyModel(){};

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override {
    return "";
  };
};

std::unique_ptr<DummyModel> getModel() {
  return std::make_unique<DummyModel>();
}

std::unique_ptr<DummyModel>
getModelWithWorkspace(std::string const &workspaceName,
                      std::size_t const &numberOfSpectra,
                      std::size_t const &numberOfBins = 3) {
  auto model = getModel();
  auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(
      numberOfSpectra, numberOfBins);
  SetUpADSWithWorkspace ads(workspaceName, workspace);
  model->addWorkspace(workspaceName);
  return model;
}

} // namespace

class IndirectFittingModelTest : public CxxTest::TestSuite {
public:
  static IndirectFittingModelTest *createSuite() {
    return new IndirectFittingModelTest();
  }

  static void destroySuite(IndirectFittingModelTest *suite) { delete suite; }

  void test_model_is_instantiated_correctly() {
    // NOT FINISHED
  }

  void test_workspace_is_stored_correctly_in_the_ADS() {
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(3, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("WorkspaceName"));
    MatrixWorkspace_sptr storedWorkspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("WorkspaceName"));
    TS_ASSERT_EQUALS(storedWorkspace->getNumberHistograms(), 3);
  }

  void test_addWorkspace_will_add_a_workspace_to_the_fittingData_correctly() {
    auto model = getModel();
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(3, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    model->addWorkspace("WorkspaceName");

    TS_ASSERT_EQUALS(model->getWorkspace(0), workspace);
  }

  void
  test_nullptr_is_returned_when_getWorkspace_is_provided_an_out_of_range_index() {
    auto model = getModelWithWorkspace("WorkspaceName", 3);
    TS_ASSERT_EQUALS(model->getWorkspace(1), nullptr);
  }

  void
  test_getSpectra_returns_a_correct_spectra_when_the_index_provided_is_valid() {
    auto model = getModelWithWorkspace("WorkspaceName", 3);

    Spectra const inputSpectra = DiscontinuousSpectra<std::size_t>("0-1");
    model->setSpectra(inputSpectra, 0);

    TS_ASSERT(boost::apply_visitor(AreSpectraEqual(), model->getSpectra(0),
                                   inputSpectra));
  }

  void
  test_getSpectra_returns_an_empty_DiscontinuousSpectra_when_provided_an_out_of_range_index() {
    auto model = getModelWithWorkspace("WorkspaceName", 3);

    Spectra const spec(DiscontinuousSpectra<std::size_t>(""));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), model->getSpectra(3), spec));
  }

  void
  test_getFittingRange_returns_correct_range_when_provided_a_valid_index_and_spectrum() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setStartX(1.2, 0, 0);
    model->setEndX(5.6, 0, 0);

    TS_ASSERT_EQUALS(model->getFittingRange(0, 0).first, 1.2);
    TS_ASSERT_EQUALS(model->getFittingRange(0, 0).second, 5.6);
  }

  void
  test_getFittingRange_returns_empty_range_when_provided_an_out_of_range_dataIndex() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setStartX(1.2, 0, 0);
    model->setEndX(5.6, 0, 0);

    TS_ASSERT_EQUALS(model->getFittingRange(1, 0).first, 0.0);
    TS_ASSERT_EQUALS(model->getFittingRange(1, 0).second, 0.0);
  }

  void test_getFittingRange_returns_empty_range_when_there_are_zero_spectra() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setStartX(1.2, 0, 0);
    model->setEndX(5.6, 0, 0);
    DiscontinuousSpectra<std::size_t> const emptySpec("");
    model->setSpectra(emptySpec, 0);

    TS_ASSERT_EQUALS(model->getFittingRange(0, 0).first, 0.0);
    TS_ASSERT_EQUALS(model->getFittingRange(0, 0).second, 0.0);
  }

  void
  test_getExcludeRegion_returns_correct_range_when_provided_a_valid_index_and_spectrum() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,3,4", 0, 0);

    TS_ASSERT_EQUALS(model->getExcludeRegion(0, 0), "0.0,1.0,3.0,4.0");
  }

  void
  test_getExcludeRegion_returns_empty_range_when_provided_an_out_of_range_dataIndex() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,3,4", 0, 0);

    TS_ASSERT_EQUALS(model->getExcludeRegion(1, 0), "");
  }

  void test_getExcludeRegion_returns_empty_range_when_there_are_zero_spectra() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,3,4", 0, 0);
    DiscontinuousSpectra<std::size_t> const emptySpec("");
    model->setSpectra(emptySpec, 0);

    TS_ASSERT_EQUALS(model->getExcludeRegion(1, 0), "");
  }

  void
  test_getExcludeRegion_returns_a_region_where_each_range_is_in_order_after_setExcludeRegion_is_given_an_unordered_region_string() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    model->setExcludeRegion("0,1,6,4", 0, 0);

    TS_ASSERT_EQUALS(model->getExcludeRegion(0, 0), "0.0,1.0,4.0,6.0");
  }

  void
  test_createDisplayName_returns_valid_string_when_provided_an_in_range_dataIndex() {
    auto model = getModelWithWorkspace("WorkspaceName", 1);

    std::string const formatString = "%1%_s%2%_Result";
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(model->createOutputName(formatString, rangeDelimiter, 0),
                     "WorkspaceName_s0_Result");
  }

  void
  test_createDisplayName_returns_string_with_red_removed_from_the_workspace_name() {
    auto model = getModelWithWorkspace("Workspace_3456_red", 1);

    std::string const formatString = "%1%_s%2%_Result";
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(model->createOutputName(formatString, rangeDelimiter, 0),
                     "Workspace_3456_s0_Result");
  }

  void
  test_createDisplayName_returns_correct_name_when_provided_a_valid_rangeDelimiter_and_formatString() {
    auto model = getModelWithWorkspace("Workspace_3456_red", 1);

    std::vector<std::string> const formatStrings{
        "%1%_s%2%_Result", "%1%_f%2%,s%2%_Parameter", "%1%_s%2%_Parameter"};
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(
        model->createOutputName(formatStrings[0], rangeDelimiter, 0),
        "Workspace_3456_s0_Result");
    TS_ASSERT_EQUALS(
        model->createOutputName(formatStrings[1], rangeDelimiter, 0),
        "Workspace_3456_f0+s0_Parameter");
    TS_ASSERT_EQUALS(
        model->createOutputName(formatStrings[2], rangeDelimiter, 0),
        "Workspace_3456_s1_Parameter");
  }
};

#endif
