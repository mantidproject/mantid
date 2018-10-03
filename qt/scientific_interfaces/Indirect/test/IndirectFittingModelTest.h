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

IndirectFitData getIndirectFitData(std::size_t const &numberOfSpectra,
                                   std::size_t const &numberOfBins) {
  auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(
      numberOfSpectra, numberOfBins);
  Spectra const spec = std::make_pair(0u, workspace->getNumberHistograms() - 1);
  IndirectFitData data(workspace, spec);
  return data;
}

/// Simple class to set up the ADS with the configuration required
struct SetUpADSWithWorkspace {

  SetUpADSWithWorkspace(std::string const &inputWSName,
                        Workspace2D_sptr const &workspace) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, workspace);
  };

  ~SetUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
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
    auto model = getModel();
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(3, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    model->addWorkspace("WorkspaceName");

    TS_ASSERT_EQUALS(model->getWorkspace(1), nullptr);
  }

  void
  test_getSpectra_returns_a_correct_spectra_when_the_index_provided_is_valid() {
    auto model = getModel();
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(3, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    model->setSpectra("0-1", 0);
    Spectra spec = std::make_pair(0u, 1u);

    TS_ASSERT_EQUALS(model->getSpectra(0), spec);
  }

  void
  test_getSpectra_returns_an_empty_DiscontinuousSpectra_when_provided_an_out_of_range_index() {
  }

  void
  test_getFittingRange_returns_correct_range_when_provided_a_valid_index_and_spectrum() {
  }

  void
  test_getFittingRange_returns_empty_range_when_provided_an_out_of_range_dataIndex() {
  }

  void test_getFittingRange_returns_empty_range_when_there_are_zero_spectra() {}

  void
  test_getFittingRange_returns_correct_range_when_the_fittingMode_is_sequential() {
  }

  void
  test_getFittingRange_returns_correct_range_when_the_fittingMode_is_not_sequential() {
  }
};

#endif
