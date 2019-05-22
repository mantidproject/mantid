// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_CREATEMODERATORMODELTEST_H_
#define MANTID_DATAHANDLING_CREATEMODERATORMODELTEST_H_

#include "MantidDataHandling/CreateModeratorModel.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/IkedaCarpenterModerator.h"

using Mantid::API::IAlgorithm_sptr;
using Mantid::DataHandling::CreateModeratorModel;

class CreateModeratorModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateModeratorModelTest *createSuite() {
    return new CreateModeratorModelTest();
  }
  static void destroySuite(CreateModeratorModelTest *suite) { delete suite; }

  CreateModeratorModelTest()
      : m_inputName("CreateModeratorModelTest"),
        m_testWS(createTestWorkspace()) {}

  void setUp() override {
    Mantid::API::AnalysisDataService::Instance().add(m_inputName, m_testWS);
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove(m_inputName);
  }

  void test_Init() {
    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm();)
    TS_ASSERT(alg->isInitialized());
  }

  void test_ModelType_Is_Not_Valid_By_Default() {
    IAlgorithm_sptr alg = createAlgorithm(m_inputName);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Algorithm_Throws_If_Moderator_Model_Is_Unknown() {
    IAlgorithm_sptr alg = createAlgorithm(m_inputName);

    TS_ASSERT_THROWS(alg->setPropertyValue("ModelType", "gibberish"),
                     const std::invalid_argument &);
  }

  void test_Setting_Parameter_String_Throws_If_It_Is_Empty() {
    IAlgorithm_sptr alg =
        createAlgorithm(m_inputName, "IkedaCarpenterModerator");

    TS_ASSERT_THROWS(alg->setPropertyValue("Parameters", ""),
                     const std::invalid_argument &);
  }

  void
  test_Setting_Parameter_String_WIth_Some_Values_Leaves_Others_As_Default() {
    doParameterCheckTest("TiltAngle=27,TauS=45", 27 * M_PI / 180.0, 0.0, 45,
                         0.0);
  }

  void test_Setting_All_Parameters_Attaches_Correct_Moderator_Object() {
    doParameterCheckTest("TiltAngle=27,TauF=13.55,TauS=45,R=0.01",
                         27 * M_PI / 180.0, 13.55, 45, 0.01);
  }

private:
  void doParameterCheckTest(const std::string &params, const double tilt,
                            const double tauf, const double taus,
                            const double rmix) {
    using namespace Mantid::API;
    IAlgorithm_sptr alg =
        createAlgorithm(m_inputName, "IkedaCarpenterModerator");

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Parameters", params));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto ws = Mantid::API::AnalysisDataService::Instance()
                  .retrieveWS<MatrixWorkspace>(m_inputName);
    TS_ASSERT(ws);
    if (ws) {
      IkedaCarpenterModerator *sourceProps =
          dynamic_cast<IkedaCarpenterModerator *>(&ws->moderatorModel());
      TSM_ASSERT("Found moderator object but it was not of the expected type",
                 sourceProps);

      TS_ASSERT_DELTA(sourceProps->getTiltAngleInRadians(), tilt, 1e-10);
      TS_ASSERT_DELTA(sourceProps->getFastDecayCoefficent(), tauf, 1e-10);
      TS_ASSERT_DELTA(sourceProps->getSlowDecayCoefficent(), taus, 1e-10);
      TS_ASSERT_DELTA(sourceProps->getMixingCoefficient(), rmix, 1e-10);
    }
  }

  Mantid::API::IAlgorithm_sptr
  createAlgorithm(const std::string &workspace = "",
                  const std::string &model = "") {
    auto alg = boost::make_shared<CreateModeratorModel>();
    alg->setRethrows(true);
    alg->initialize();

    if (!workspace.empty())
      alg->setPropertyValue("Workspace", workspace);
    if (!model.empty())
      alg->setPropertyValue("ModelType", model);

    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspace() {
    using Mantid::API::MatrixWorkspace_sptr;
    return WorkspaceCreationHelper::create2DWorkspace(1, 10);
  }

  const std::string m_inputName;
  Mantid::API::MatrixWorkspace_sptr m_testWS;
};

#endif /* MANTID_DATAHANDLING_CREATEMODERATORMODELTEST_H_ */
