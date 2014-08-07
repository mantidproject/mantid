#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>

using Mantid::Algorithms::CreateTransmissionWorkspaceAuto;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace boost::assign;
using Mantid::MantidVec;

namespace
{
  class PropertyFinder
  {
  private:
    const std::string m_propertyName;
  public:
    PropertyFinder(const std::string& propertyName) :
        m_propertyName(propertyName)
    {
    }
    bool operator()(const PropertyHistories::value_type& candidate) const
    {
      return candidate->name() == m_propertyName;
    }
  };

  template<typename T>
  T findPropertyValue(PropertyHistories& histories, const std::string& propertyName)
  {
    PropertyFinder finder(propertyName);
    auto it = std::find_if(histories.begin(), histories.end(), finder);
    return boost::lexical_cast<T>((*it)->value());
  }
}

class CreateTransmissionWorkspaceAutoTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspaceAutoTest *createSuite()
  {
    return new CreateTransmissionWorkspaceAutoTest();
  }
  static void destroySuite(CreateTransmissionWorkspaceAutoTest *suite)
  {
    delete suite;
  }


  MatrixWorkspace_sptr m_dataWS;

  CreateTransmissionWorkspaceAutoTest()
  {
    FrameworkManager::Instance();

    IAlgorithm_sptr lAlg = AlgorithmManager::Instance().create("Load");
    lAlg->setChild(true);
    lAlg->initialize();
    lAlg->setProperty("Filename", "INTER00013460.nxs");
    lAlg->setPropertyValue("OutputWorkspace", "demo_ws");
    lAlg->execute();
    Workspace_sptr temp = lAlg->getProperty("OutputWorkspace");
    m_dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

  }

  ~CreateTransmissionWorkspaceAutoTest()
  {
  }


  void test_Init()
  {
    CreateTransmissionWorkspaceAuto alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_exec()
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->initialize());
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("FirstTransmissionRun", m_dataWS));
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "outWS"));
    alg->execute();
    TS_ASSERT( alg->isExecuted());

    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");

    auto inst = m_dataWS->getInstrument();
    auto workspaceHistory = outWS->getHistory();
    AlgorithmHistory_const_sptr workerAlgHistory =
        workspaceHistory.getAlgorithmHistory(0)->getChildAlgorithmHistory(0);
    auto vecPropertyHistories = workerAlgHistory->getProperties();

    const double wavelengthMin = findPropertyValue<double>(vecPropertyHistories, "WavelengthMin");
    double wavelengthMax = findPropertyValue<double>(vecPropertyHistories, "WavelengthMax");
    double monitorBackgroundWavelengthMin = findPropertyValue<double>(vecPropertyHistories,
        "MonitorBackgroundWavelengthMin");
    double monitorBackgroundWavelengthMax = findPropertyValue<double>(vecPropertyHistories,
        "MonitorBackgroundWavelengthMax");
    double monitorIntegrationWavelengthMin = findPropertyValue<double>(vecPropertyHistories,
        "MonitorIntegrationWavelengthMin");
    double monitorIntegrationWavelengthMax = findPropertyValue<double>(vecPropertyHistories,
        "MonitorIntegrationWavelengthMax");
    int i0MonitorIndex = findPropertyValue<int>(vecPropertyHistories, "I0MonitorIndex");
    std::string processingInstructions = findPropertyValue<std::string>(vecPropertyHistories,
        "ProcessingInstructions");
    std::vector<std::string> pointDetectorStartStop;
    boost::split(pointDetectorStartStop, processingInstructions, boost::is_any_of(","));

    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMin").at(0), wavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("LambdaMax").at(0), wavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMin").at(0),
        monitorBackgroundWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorBackgroundMax").at(0),
        monitorBackgroundWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMin").at(0),
        monitorIntegrationWavelengthMin);
    TS_ASSERT_EQUALS(inst->getNumberParameter("MonitorIntegralMax").at(0),
        monitorIntegrationWavelengthMax);
    TS_ASSERT_EQUALS(inst->getNumberParameter("I0MonitorIndex").at(0), i0MonitorIndex);
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStart").at(0),
        boost::lexical_cast<double>(pointDetectorStartStop.at(0)));
    TS_ASSERT_EQUALS(inst->getNumberParameter("PointDetectorStop").at(0),
        boost::lexical_cast<double>(pointDetectorStartStop.at(1)));

  }


};

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTOTEST_H_ */
