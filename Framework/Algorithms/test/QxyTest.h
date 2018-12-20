#ifndef QXYTEST_H_
#define QXYTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/Qxy.h"
#include "MantidDataHandling/LoadRaw3.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class QxyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QxyTest *createSuite() { return new QxyTest(); }
  static void destroySuite(QxyTest *suite) { delete suite; }

  QxyTest() : m_inputWS("QxyTest_input_in_wav") {}

  void testName() { TS_ASSERT_EQUALS(qxy.name(), "Qxy") }

  void testVersion() { TS_ASSERT_EQUALS(qxy.version(), 1) }

  void testCategory() { TS_ASSERT_EQUALS(qxy.category(), "SANS") }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(qxy.initialize())
    TS_ASSERT(qxy.isInitialized())
  }

  void testNoGravity() {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48098.raw");
    loader.setPropertyValue("OutputWorkspace", m_inputWS);
    loader.setPropertyValue("SpectrumMin", "30");
    loader.setPropertyValue("SpectrumMax", "130");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace", m_inputWS);
    convert.setPropertyValue("OutputWorkspace", m_inputWS);
    convert.setPropertyValue("Target", "Wavelength");
    convert.execute();

    if (!qxy.isInitialized())
      qxy.initialize();

    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("InputWorkspace", m_inputWS))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("MaxQxy", "0.1"))
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("DeltaQ", "0.002"))
    TS_ASSERT_THROWS_NOTHING(qxy.setProperty("OutputParts", true))
    TS_ASSERT_THROWS_NOTHING(qxy.execute())
    TS_ASSERT(qxy.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 100)
    TS_ASSERT_EQUALS(result->blocksize(), 100)
    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "MomentumTransfer")
    TS_ASSERT_EQUALS(result->getAxis(1)->unit()->unitID(), "MomentumTransfer")
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(0), -0.1)
    TS_ASSERT_DELTA((*(result->getAxis(1)))(31), -0.038, 0.001)
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(100), 0.1)

    TS_ASSERT_EQUALS(result->x(0).size(), 101)
    TS_ASSERT_EQUALS(result->x(0).front(), -0.1)
    TS_ASSERT_DELTA(result->x(0)[64], 0.028, 0.01)
    TS_ASSERT_DELTA(result->x(0).back(), 0.1, 1e-14)

    TS_ASSERT_DIFFERS(result->y(0).front(), result->y(0).front()) // NaN
    TS_ASSERT_DELTA(result->y(28)[71], 229914.7, 1)
    TS_ASSERT_DELTA(result->y(26)[73], 0.0, 1)
    TS_ASSERT_DELTA(result->y(18)[80], 344640.4, 1)

    TS_ASSERT_DELTA(result->e(20)[67], 0.0, 1e-3)
    TS_ASSERT_DELTA(result->e(27)[70], 114778.1004, 1)
    TS_ASSERT_DELTA(result->e(18)[80], 344640, 1)

    Mantid::API::MatrixWorkspace_sptr sumOfCounts;
    TS_ASSERT_THROWS_NOTHING(
        sumOfCounts = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                outputWS + "_sumOfCounts")))

    Mantid::API::MatrixWorkspace_sptr sumOfNormFactors;
    TS_ASSERT_THROWS_NOTHING(
        sumOfNormFactors =
            boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                Mantid::API::AnalysisDataService::Instance().retrieve(
                    outputWS + "_sumOfNormFactors")))

    TS_ASSERT_DELTA(sumOfCounts->y(28)[71], 2.0000, 0.01)
    TS_ASSERT_DELTA(sumOfNormFactors->y(28)[71], 8.6988767154375003e-006,
                    0.00000001)

    TS_ASSERT_DELTA(sumOfCounts->e(28)[71], 1.4142135623730951, 0.01)
    TS_ASSERT_DELTA(sumOfNormFactors->e(28)[71], 0.0, 0.00000001)

    TS_ASSERT_EQUALS(sumOfCounts->getNumberHistograms(), 100)
    TS_ASSERT_EQUALS(sumOfCounts->blocksize(), 100)
    TS_ASSERT_EQUALS(sumOfNormFactors->getNumberHistograms(), 100)
    TS_ASSERT_EQUALS(sumOfNormFactors->blocksize(), 100)

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testGravity() {
    Mantid::Algorithms::Qxy qxy;
    qxy.initialize();

    // inputWS was set up by the previous test, not ideal but it does save a lot
    // of CPU time!
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("InputWorkspace", m_inputWS))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("MaxQxy", "0.1"))
    TS_ASSERT_THROWS_NOTHING(qxy.setPropertyValue("DeltaQ", "0.002"))
    TS_ASSERT_THROWS_NOTHING(qxy.setProperty("AccountForGravity", true))

    TS_ASSERT_THROWS_NOTHING(qxy.execute())
    TS_ASSERT(qxy.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 100)
    TS_ASSERT_EQUALS(result->blocksize(), 100)
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(0), -0.1)
    TS_ASSERT_DELTA((*(result->getAxis(1)))(31), -0.038, 0.001)
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(100), 0.1)

    TS_ASSERT_DIFFERS(result->y(0).front(), result->y(0).front()) // NaN
    TS_ASSERT_DELTA(result->y(3)[26], 0.0000, 1)
    TS_ASSERT_DELTA(result->y(6)[51], 341936, 1)
    TS_ASSERT_DELTA(result->y(7)[27], 685501, 1)

    TS_ASSERT_DELTA(result->e(20)[67], 0.0, 1e-3)
    TS_ASSERT_DELTA(result->e(7)[27], 685500.615, 1e-3)
    TS_ASSERT_DELTA(result->e(23)[34], 0.0, 1e-3)

    Mantid::API::AnalysisDataService::Instance().remove(m_inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::Qxy qxy;
  const std::string m_inputWS;
};

class QxyTestPerformance : public CxxTest::TestSuite {
public:
  std::string m_inputWS;
  std::string m_outputWS;

  QxyTestPerformance()
      : m_inputWS("QxyTest_input_in_wav"), m_outputWS("result") {}

  void setUp() override {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48098.raw");
    loader.setPropertyValue("OutputWorkspace", m_inputWS);
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace", m_inputWS);
    convert.setPropertyValue("OutputWorkspace", m_inputWS);
    convert.setPropertyValue("Target", "Wavelength");
    convert.execute();
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove(m_outputWS);
  }

  void test_slow_performance() {
    Mantid::Algorithms::Qxy qxy;
    qxy.initialize();
    qxy.setPropertyValue("InputWorkspace", m_inputWS);
    qxy.setPropertyValue("OutputWorkspace", m_outputWS);
    qxy.setPropertyValue("MaxQxy", "0.1");
    qxy.setPropertyValue("DeltaQ", "0.002");
    qxy.setProperty("OutputParts", true);
    qxy.execute();
  }
};

#endif /*QXYTEST_H_*/
