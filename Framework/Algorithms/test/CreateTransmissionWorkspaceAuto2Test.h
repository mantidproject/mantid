#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO2TEST_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO2TEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto2.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PropertyHistory.h"
#include <boost/lexical_cast.hpp>
#include <cxxtest/TestSuite.h>

using Mantid::Algorithms::CreateTransmissionWorkspaceAuto2;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
class PropertyFinder {
private:
  const std::string m_propertyName;

public:
  PropertyFinder(const std::string &propertyName)
      : m_propertyName(propertyName) {}
  bool operator()(const PropertyHistories::value_type &candidate) const {
    return candidate->name() == m_propertyName;
  }
};

template <typename T>
T findPropertyValue(PropertyHistories &histories,
                    const std::string &propertyName) {
  PropertyFinder finder(propertyName);
  auto it = std::find_if(histories.begin(), histories.end(), finder);
  return boost::lexical_cast<T>((*it)->value());
}
} // namespace

class CreateTransmissionWorkspaceAuto2Test : public CxxTest::TestSuite {

private:
  MatrixWorkspace_sptr m_dataWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspaceAuto2Test *createSuite() {
    return new CreateTransmissionWorkspaceAuto2Test();
  }
  static void destroySuite(CreateTransmissionWorkspaceAuto2Test *suite) {
    delete suite;
  }

  CreateTransmissionWorkspaceAuto2Test() {
    FrameworkManager::Instance();

    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->setChild(true);
    lAlg->initialize();
    lAlg->setProperty("Filename", "INTER00013463.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "demo_ws");
    lAlg->execute();
    Workspace_sptr temp = lAlg->getProperty("OutputWorkspace");
    m_dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
  }

  ~CreateTransmissionWorkspaceAuto2Test() override {}

  void test_init() {
    CreateTransmissionWorkspaceAuto2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    alg->setRethrows(true);
    alg->initialize();

    alg->setProperty("FirstTransmissionRun", m_dataWS);
    alg->setPropertyValue("OutputWorkspace", "outWS");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");

    const auto workspaceHistory = outWS->getHistory();
    AlgorithmHistory_const_sptr workerAlgHistory =
        workspaceHistory.getAlgorithmHistory(0)->getChildAlgorithmHistory(0);
    auto vecPropertyHistories = workerAlgHistory->getProperties();

    const double startOverlap =
        findPropertyValue<double>(vecPropertyHistories, "StartOverlap");
    const double endOverlap =
        findPropertyValue<double>(vecPropertyHistories, "EndOverlap");
    const double wavelengthMin =
        findPropertyValue<double>(vecPropertyHistories, "WavelengthMin");
    const double wavelengthMax =
        findPropertyValue<double>(vecPropertyHistories, "WavelengthMax");
    const double monitorBackgroundWavelengthMin = findPropertyValue<double>(
        vecPropertyHistories, "MonitorBackgroundWavelengthMin");
    const double monitorBackgroundWavelengthMax = findPropertyValue<double>(
        vecPropertyHistories, "MonitorBackgroundWavelengthMax");
    const double monitorIntegrationWavelengthMin = findPropertyValue<double>(
        vecPropertyHistories, "MonitorIntegrationWavelengthMin");
    const double monitorIntegrationWavelengthMax = findPropertyValue<double>(
        vecPropertyHistories, "MonitorIntegrationWavelengthMax");
    const int i0MonitorIndex =
        findPropertyValue<int>(vecPropertyHistories, "I0MonitorIndex");
    const std::string processingInstructions = findPropertyValue<std::string>(
        vecPropertyHistories, "ProcessingInstructions");
    std::vector<std::string> pointDetectorStartStop;
    boost::split(pointDetectorStartStop, processingInstructions,
                 boost::is_any_of(":"));

    auto inst = m_dataWS->getInstrument();
    TS_ASSERT_EQUALS(inst->getNumberParameter("TransRunStartOverlap").at(0),
                     startOverlap);
    TS_ASSERT_EQUALS(inst->getNumberParameter("TransRunEndOverlap").at(0),
                     endOverlap);
    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMin").at(0),
                     wavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMax").at(0),
                     wavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMin").at(0),
                     monitorBackgroundWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMax").at(0),
                     monitorBackgroundWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMin").at(0),
                     monitorIntegrationWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMax").at(0),
                     monitorIntegrationWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("I0MonitorIndex").at(0),
                     i0MonitorIndex);
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStart").at(0),
                     boost::lexical_cast<double>(pointDetectorStartStop.at(0)));
    TS_ASSERT_EQUALS(pointDetectorStartStop.size(), 1);

    AnalysisDataService::Instance().remove("outWS");
  }
};

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO2TEST_H_ */
