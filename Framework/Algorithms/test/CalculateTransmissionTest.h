#ifndef CALCULATETRANSMISSIONTEST_H_
#define CALCULATETRANSMISSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/SANSInstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;
using Mantid::API::MatrixWorkspace;

class CalculateTransmissionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateTransmissionTest *createSuite() {
    return new CalculateTransmissionTest();
  }
  static void destroySuite(CalculateTransmissionTest *suite) { delete suite; }

  void testBasics() {
    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_EQUALS(trans.name(), "CalculateTransmission");
    TS_ASSERT_EQUALS(trans.version(), 1);
  }

  void testDefiningBothTransmissionMonitorAndRegionOfInterestThrows() {
    Mantid::Algorithms::CalculateTransmission trans;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        4, 50, true);
    inputWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    const std::string outputWsName("CalculateTransmissionTest_outputWS");

    trans.initialize();

    TS_ASSERT_THROWS_NOTHING(trans.setProperty("SampleRunWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("DirectRunWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("IncidentBeamMonitor", 1));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("TransmissionMonitor", 2));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("TransmissionROI", "2,3"));
    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("OutputWorkspace", outputWsName));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("OutputUnfittedData", true));

    trans.execute();

    TS_ASSERT(!trans.isExecuted());
  }

  void testRegionOfInterest() {
    Mantid::Algorithms::CalculateTransmission trans;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        4, 50, true);
    inputWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    const std::string outputWsName("CalculateTransmissionTest_outputWS");

    trans.initialize();

    TS_ASSERT_THROWS_NOTHING(trans.setProperty("SampleRunWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("DirectRunWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("IncidentBeamMonitor", 1));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("TransmissionROI", "2,3"));
    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("OutputWorkspace", outputWsName));
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("OutputUnfittedData", true));

    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr fitted, unfitted;
    TS_ASSERT_THROWS_NOTHING(
        fitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                outputWsName)));
    TS_ASSERT_THROWS_NOTHING(
        unfitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                outputWsName + "_unfitted")));

    auto &fit = fitted->y(0);
    auto &unfit = unfitted->y(0);
    TS_ASSERT_EQUALS(fit.size(), unfit.size())
    for (unsigned int i = 0; i < fit.size(); ++i) {
      // Should all be 1 because I used the same workspace twice as the input
      TS_ASSERT_DELTA(fit[i], 1.0, 0.0005)
      // a linear fit thorugh all 1s should result in all 1s
      TS_ASSERT_DELTA(fit[i], unfit[i], 0.0005)
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWsName);
    Mantid::API::AnalysisDataService::Instance().remove(outputWsName +
                                                        "_unfitted");
  }

  void testFittedUnfitted() {

    Mantid::API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            4, 50, true); // Make sure this has a spectrum 3
    inputWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_THROWS_NOTHING(trans.initialize());
    TS_ASSERT(trans.isInitialized());

    TS_ASSERT_THROWS_NOTHING(trans.setProperty("SampleRunWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("DirectRunWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("IncidentBeamMonitor", 2))
    std::string outputWS("CalculateTransmissionTest_outputWS");
    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("OutputUnfittedData", true))

    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr fitted, unfitted;
    TS_ASSERT_THROWS_NOTHING(
        fitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_THROWS_NOTHING(
        unfitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                outputWS + "_unfitted")));

    auto &fit = fitted->y(0);
    auto &unfit = unfitted->y(0);
    TS_ASSERT_EQUALS(fit.size(), unfit.size())
    for (unsigned int i = 0; i < fit.size(); ++i) {
      // Should all be 1 because I used the same workspace twice as the input
      TS_ASSERT_DELTA(fit[i], 1.0, 0.0005)
      // a linear fit thorugh all 1s should result in all 1s
      TS_ASSERT_DELTA(fit[i], unfit[i], 0.0005)
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_unfitted");
  }

  void testFittedNoBeamMonitor() {

    Mantid::API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            4, 50, true); // Make sure this has a spectrum 3
    inputWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CalculateTransmission trans;

    TS_ASSERT_THROWS_NOTHING(trans.initialize());
    TS_ASSERT(trans.isInitialized());

    TS_ASSERT_THROWS_NOTHING(trans.setProperty("SampleRunWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("DirectRunWorkspace", inputWS))
    std::string outputWS("CalculateTransmissionTest_outputWS");
    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("OutputUnfittedData", false))

    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr fitted, unfitted;
    TS_ASSERT_THROWS_NOTHING(
        fitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));

    auto &fit = fitted->y(0);
    for (unsigned int i = 0; i < fit.size(); ++i) {
      // Should all be 1 because I used the same workspace twice as the input
      TS_ASSERT_DELTA(fit[i], 1.0, 0.0005)
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testSingleBin() {
    // Create an test workspace with a single wavelength bin and test that
    // the algorithm completes.

    const std::string inputWS = "sampletransdata";

    Mantid::DataObjects::Workspace2D_sptr ws =
        SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWS, ws);

    const std::string emptyWS("directbeam_ws");
    Mantid::DataObjects::Workspace2D_sptr empty_ws =
        SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(emptyWS);

    // According to this detector geometry, Monitor #1 is spectrum 0, and
    // Monitor #2 is spectrum 1.
    empty_ws->mutableY(0)[0] = 10.0;
    Mantid::API::AnalysisDataService::Instance().addOrReplace(emptyWS,
                                                              empty_ws);

    TS_ASSERT_EQUALS(ws->y(0).size(), 1)

    Mantid::Algorithms::CalculateTransmission trans;
    TS_ASSERT_THROWS_NOTHING(trans.initialize());

    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("SampleRunWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("DirectRunWorkspace", emptyWS))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("IncidentBeamMonitor", 1))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("TransmissionMonitor", 2))
    std::string outputWS("CalculateTransmissionTest_outputWS2");
    TS_ASSERT_THROWS_NOTHING(
        trans.setPropertyValue("OutputWorkspace", outputWS))

    trans.execute();
    TS_ASSERT(trans.isExecuted())

    Mantid::API::MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_DELTA(output->y(0)[0], 5.0, 0.005)

    // If we reverse the monitors, we should invert the output
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("IncidentBeamMonitor", 2))
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("TransmissionMonitor", 1))
    trans.execute();
    TS_ASSERT_THROWS_NOTHING(
        output = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_DELTA(output->y(0)[0], 0.2, 0.005)

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(emptyWS);
  }
  /// this tests where the output ranges is greater than the input range
  void testExtrapolationFit() {
    CalculateTransmission trans;
    trans.initialize();
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setPropertyValue("OutputWorkspace",
                           "CalculateTransmissionTest_extra");
    trans.setProperty("IncidentBeamMonitor", 1);
    trans.setProperty("TransmissionMonitor", 2);
    trans.setProperty("RebinParams", "0.5, 0.1, 14");

    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr extra =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "CalculateTransmissionTest_extra"));

    // these values were dervived from the debugger when exprolation was first
    // added and are believed to be correct on that basis
    TS_ASSERT_DELTA(extra->y(0)[0], 0.2, 0.8937)
    TS_ASSERT_DELTA(extra->y(0)[8], 0.2, 0.8801)
    TS_ASSERT_DELTA(extra->y(0)[18], 0.2, 0.8634)
    TS_ASSERT_DELTA(extra->y(0)[33], 0.2, 0.8390)
    TS_ASSERT_DELTA(extra->y(0)[54], 0.2, 0.8059)
    TS_ASSERT_DELTA(extra->y(0).back(), 0.2, 0.6914)

    Mantid::API::AnalysisDataService::Instance().remove(
        "CalculateTransmissionTest_extra");
  }

  /// fitting with log or linear should give similar results
  void testLogLin() {
    CalculateTransmission trans;
    trans.initialize();
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setPropertyValue("OutputWorkspace", "CalculateTransmissionTest_log");
    trans.setProperty("IncidentBeamMonitor", 1);
    trans.setProperty("TransmissionMonitor", 2);
    trans.setProperty("RebinParams", "0.8, 0.1, 8");
    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setProperty("FitMethod", "Linear");
    trans.setPropertyValue("OutputWorkspace",
                           "CalculateTransmissionTest_linear");
    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());
    Mantid::API::MatrixWorkspace_const_sptr logged, lineared;
    TS_ASSERT_THROWS_NOTHING(
        logged = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "CalculateTransmissionTest_log")));
    TS_ASSERT_THROWS_NOTHING(
        lineared = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "CalculateTransmissionTest_linear")));

    auto &log = logged->y(0);
    auto &linear = lineared->y(0);

    TS_ASSERT_EQUALS(log.size(), linear.size())
    for (unsigned int i = 0; i < linear.size(); ++i) {
      // these are not expected to match exactly but, for sensible data, they
      // should be close
      TS_ASSERT_DELTA(log[i] / linear[i], 1.0, 0.02)
    }

    Mantid::API::AnalysisDataService::Instance().remove(
        "CalculateTransmissionTest_log");
    Mantid::API::AnalysisDataService::Instance().remove(
        "CalculateTransmissionTest_linear");
  }

  void testPolyFit() {
    CalculateTransmission trans;
    trans.initialize();
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setProperty("IncidentBeamMonitor", 1);
    trans.setProperty("TransmissionMonitor", 2);
    trans.setProperty("FitMethod", "Polynomial");
    trans.setProperty("PolynomialOrder", 3);
    std::string outputWS("CalculateTransmissionTest_poly");
    trans.setPropertyValue("OutputWorkspace", outputWS);
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("OutputUnfittedData", true))
    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr fitted, unfitted;

    TS_ASSERT_THROWS_NOTHING(
        fitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_THROWS_NOTHING(
        unfitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                outputWS + "_unfitted")));

    {
      auto &fitted_y = fitted->y(0);
      auto &fitted_x = fitted->x(0);

      //  TS_ASSERT_EQUALS(fitted_y.size(), unfitted_y.size());
      double x;

      for (unsigned int i = 0; i < fitted_y.size(); ++i) {
        x = fitted_x[i]; //(fitted_x[i] + fitted_x[i+1])* 0.5;
        TS_ASSERT_DELTA(fitted_y[i],
                        26.6936 - 9.31494 * x + 1.11532 * x * x -
                            0.044502 * x * x * x,
                        0.01);
      }
    }
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_unfitted");
  }
  void testPolyFitWithRebin() {
    CalculateTransmission trans;
    trans.initialize();
    trans.setPropertyValue("SampleRunWorkspace", m_transWS);
    trans.setPropertyValue("DirectRunWorkspace", m_dirWS);
    trans.setProperty("IncidentBeamMonitor", 1);
    trans.setProperty("TransmissionMonitor", 2);
    trans.setProperty("FitMethod", "Polynomial");
    trans.setProperty("PolynomialOrder", 3);
    trans.setProperty("RebinParams", "7.5, 0.1, 9");
    std::string outputWS("CalculateTransmissionTest_poly2");
    trans.setPropertyValue("OutputWorkspace", outputWS);
    TS_ASSERT_THROWS_NOTHING(trans.setProperty("OutputUnfittedData", true))
    TS_ASSERT_THROWS_NOTHING(trans.execute());
    TS_ASSERT(trans.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr fitted, unfitted;

    TS_ASSERT_THROWS_NOTHING(
        fitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_THROWS_NOTHING(
        unfitted = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                outputWS + "_unfitted")));

    {
      auto &fitted_y = fitted->y(0);
      auto &fitted_x = fitted->x(0);

      //  TS_ASSERT_EQUALS(fitted_y.size(), unfitted_y.size());
      double x;

      for (unsigned int i = 0; i < fitted_y.size(); ++i) {
        x = (fitted_x[i] + fitted_x[i + 1]) * 0.5;
        TS_ASSERT_DELTA(fitted_y[i],
                        26.6936 - 9.31494 * x + 1.11532 * x * x -
                            0.044502 * x * x * x,
                        0.01);
      }
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS + "_unfitted");
  }

  CalculateTransmissionTest()
      : m_dirWS("CalculateTransmissionTest_direct"),
        m_transWS("CalculateTransmissionTest_trans") {
    loadSampleLOQMonitors();
  }

  ~CalculateTransmissionTest() override {
    Mantid::API::AnalysisDataService::Instance().remove(m_dirWS);
    Mantid::API::AnalysisDataService::Instance().remove(m_transWS);
  }

  /// Load and convert some monitor spectra to create some non-trival input data
  void loadSampleLOQMonitors() {
    // load a couple of real montior spectra
    std::string wkspName("LOQ48097");
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48097.raw");
    loader.setPropertyValue("OutputWorkspace", wkspName);
    loader.setProperty("SpectrumMin", 1);
    loader.setProperty("SpectrumMax", 2);
    loader.execute();
    // convert it to wavelength
    ConvertUnits unis;
    unis.initialize();
    unis.setPropertyValue("InputWorkspace", wkspName);
    unis.setPropertyValue("OutputWorkspace", wkspName);
    unis.setProperty("Target", "Wavelength");
    unis.execute();
    // crop off prompt spikes
    Rebin crop;
    crop.initialize();
    crop.setPropertyValue("InputWorkspace", wkspName);
    crop.setPropertyValue("OutputWorkspace", m_dirWS);
    crop.setProperty("Params", "6, 0.01, 7.5");
    crop.execute();
    crop.setPropertyValue("InputWorkspace", wkspName);
    crop.setPropertyValue("OutputWorkspace", m_transWS);
    crop.setProperty("Params", "7.5, 0.01, 9");
    crop.execute();

    Mantid::API::AnalysisDataService::Instance().remove(wkspName);

    Mantid::API::MatrixWorkspace_sptr
        dir = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(m_dirWS)),
        source = boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(m_transWS));

    dir->setSharedX(0, source->sharedX(0));
    dir->setSharedX(1, source->sharedX(0));
  }

private:
  /// these are the names of some sample data workspaces
  std::string m_dirWS, m_transWS;
};

#endif /*CALCULATETRANSMISSIONTEST_H_*/
