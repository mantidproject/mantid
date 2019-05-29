// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_CREATECHOPPERMODELTEST_H_
#define MANTID_DATAHANDLING_CREATECHOPPERMODELTEST_H_

#include "MantidAPI/FermiChopperModel.h"
#include "MantidDataHandling/CreateChopperModel.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::DataHandling::CreateChopperModel;

class CreateChopperModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateChopperModelTest *createSuite() {
    return new CreateChopperModelTest();
  }
  static void destroySuite(CreateChopperModelTest *suite) { delete suite; }

  CreateChopperModelTest()
      : m_inputName("CreateChopperModelTest"), m_testWS(createTestWorkspace()) {
  }

  void setUp() override {
    Mantid::API::AnalysisDataService::Instance().add(m_inputName, m_testWS);
  }

  void tearDown() override {
    clearInstrumentFromTestWorkspace();
    Mantid::API::AnalysisDataService::Instance().remove(m_inputName);
  }

  void test_Init() {
    Mantid::API::IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
    TS_ASSERT(alg->isInitialized())
  }

  void test_ModelType_Is_Not_Valid_By_Default() {
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm(m_inputName);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Algorithm_Throws_If_Chopper_Model_Is_Unknown() {
    Mantid::API::IAlgorithm_sptr alg = createAlgorithm(m_inputName);

    TS_ASSERT_THROWS(alg->setPropertyValue("ModelType", "gibberish"),
                     const std::invalid_argument &);
  }

  void test_Setting_Parameter_String_Throws_If_It_Is_Empty() {
    Mantid::API::IAlgorithm_sptr alg =
        createAlgorithm(m_inputName, "FermiChopperModel");

    TS_ASSERT_THROWS(alg->setPropertyValue("Parameters", ""),
                     const std::invalid_argument &);
  }

  void
  test_Setting_Valid_Parameter_String_Using_Numerical_Values_Attaches_Chopper_Object() {
    runTestWithValidParameters("AngularVelocity=150,ChopperRadius=0.049,"
                               "SlitThickness=0.00228,SlitRadius=1.3,Ei=45.0");
  }

  void
  test_Setting_Valid_Parameter_String_Using_Log_Values_Attaches_Chopper_Object() {
    runTestWithValidParameters("AngularVelocity=ChopperSpeed,ChopperRadius=0."
                               "049,SlitThickness=0.00228,SlitRadius=1.3,Ei="
                               "Ei");
  }

private:
  void runTestWithValidParameters(const std::string &params) {
    using namespace Mantid::API;
    Mantid::API::IAlgorithm_sptr alg =
        createAlgorithm(m_inputName, "FermiChopperModel");
    addChopperPointToTestWorkspace();

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Parameters", params));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto ws = Mantid::API::AnalysisDataService::Instance()
                  .retrieveWS<MatrixWorkspace>(m_inputName);
    TS_ASSERT(ws);
    if (ws) {
      FermiChopperModel *chopper =
          dynamic_cast<FermiChopperModel *>(&ws->chopperModel(0));
      TSM_ASSERT("Found chopper object but it was not of the expected type",
                 chopper);
      TS_ASSERT_DELTA(chopper->pulseTimeVariance(), 1.02729824e-10, 1e-14);
    }
  }

  Mantid::API::IAlgorithm_sptr
  createAlgorithm(const std::string &workspace = "",
                  const std::string &model = "") {
    auto alg = boost::make_shared<CreateChopperModel>();
    alg->setRethrows(true);
    alg->initialize();

    if (!workspace.empty())
      alg->setPropertyValue("Workspace", workspace);
    if (!model.empty())
      alg->setPropertyValue("ModelType", model);

    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspace() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    ws->mutableRun().addProperty("Ei", 45.0);
    ws->mutableRun().addProperty("ChopperSpeed", 150.0);
    return ws;
  }

  void addChopperPointToTestWorkspace() {
    using namespace Mantid::API;
    Mantid::Geometry::Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);
    auto chopperPoint = new Mantid::Geometry::ObjComponent("chopperPoint");
    inst->add(chopperPoint);
    inst->markAsChopperPoint(chopperPoint);

    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_inputName);
    ws->setInstrument(inst);
  }

  void clearInstrumentFromTestWorkspace() {
    using namespace Mantid::API;
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_inputName);
    ws->setInstrument(
        Mantid::Geometry::Instrument_sptr(new Mantid::Geometry::Instrument));
  }

  const std::string m_inputName;
  Mantid::API::MatrixWorkspace_sptr m_testWS;
};

#endif /* MANTID_DATAHANDLING_CREATECHOPPERMODELTEST_H_ */
