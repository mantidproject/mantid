#ifndef MANTID_INDIRECTFITTINGMODELTEST_H_
#define MANTID_INDIRECTFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitData.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

/// Simple class to set up the ADS with the configuration required
struct SetUpADSWithWorkspace {

  SetUpADSWithWorkspace(std::string const &inputWSName,
                        Workspace2D_sptr const &workspace) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, workspace);
  };

  ~SetUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
};

} // namespace

class IndirectFittingModelTest : public CxxTest::TestSuite {
public:
  static IndirectFittingModelTest *createSuite() {
    return new IndirectFittingModelTest();
  }

  static void destroySuite(IndirectFittingModelTest *suite) { delete suite; }

  void test_data_is_instantiated_correctly() {}

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
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(3, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    m_model->addWorkspace("WorkspaceName");

    TS_ASSERT_EQUALS(m_model->getWorkspace(0), workspace);
  }

  void
  test_nullptr_is_returned_when_getWorkspace_provided_out_of_range_index() {
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace123(3, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", workspace);

    m_model->addWorkspace("WorkspaceName");

    TS_ASSERT_EQUALS(m_model->getWorkspace(1), nullptr);
  }

private:
  IndirectFittingModel *m_model;
};

#endif
