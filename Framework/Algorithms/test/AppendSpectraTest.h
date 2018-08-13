#ifndef MANTID_ALGORITHMS_APPENDSPECTRATEST_H_
#define MANTID_ALGORITHMS_APPENDSPECTRATEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/AppendSpectra.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

class AppendSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AppendSpectraTest *createSuite() { return new AppendSpectraTest(); }
  static void destroySuite(AppendSpectraTest *suite) { delete suite; }

  AppendSpectraTest()
      : ws1Name("ConjoinWorkspacesTest_grp1"),
        ws2Name("ConjoinWorkspacesTest_grp2") {}

  void setupWS() {
    IAlgorithm *loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "top");
    loader->setPropertyValue("SpectrumMin", "1");
    loader->setPropertyValue("SpectrumMax", "10");
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
    delete loader;

    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "bottom");
    loader->setPropertyValue("SpectrumMin", "11");
    loader->setPropertyValue("SpectrumMax", "25");
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
    delete loader;
  }

  //----------------------------------------------------------------------------------------------
  void testExec() {
    setupWS();

    AppendSpectra alg;
    if (!alg.isInitialized())
      alg.initialize();

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("top");
    MatrixWorkspace_sptr in2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("bottom");

    // Mask a spectrum and check it is carried over
    const size_t maskTop(5), maskBottom(10);
    in1->getSpectrum(maskTop).clearData();
    in2->getSpectrum(maskBottom).clearData();
    in1->mutableSpectrumInfo().setMasked(maskTop, true);
    in2->mutableSpectrumInfo().setMasked(maskBottom, true);

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace1", "top"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace2", "bottom"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "top"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("top"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 25);
    // Check a few values
    TS_ASSERT_EQUALS(output->readX(0)[0], in1->readX(0)[0]);
    TS_ASSERT_EQUALS(output->readX(15)[444], in2->readX(5)[444]);
    TS_ASSERT_EQUALS(output->readY(3)[99], in1->readY(3)[99]);
    TS_ASSERT_EQUALS(output->readE(7)[700], in1->readE(7)[700]);
    TS_ASSERT_EQUALS(output->readY(19)[55], in2->readY(9)[55]);
    TS_ASSERT_EQUALS(output->readE(10)[321], in2->readE(0)[321]);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(5),
                     in1->getAxis(1)->spectraNo(5));
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(12),
                     in2->getAxis(1)->spectraNo(2));

    // Check masking
    TS_ASSERT_EQUALS(output->spectrumInfo().isMasked(maskTop), true);
    TS_ASSERT_EQUALS(output->spectrumInfo().isMasked(10 + maskBottom), true);
  }

  //----------------------------------------------------------------------------------------------
  void testExecNumber() {
    setupWS();

    AppendSpectra alg;
    if (!alg.isInitialized())
      alg.initialize();

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("top");
    MatrixWorkspace_sptr in2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("bottom");

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace1", "top"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace2", "bottom"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "top"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Number", 2));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("top"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 40);
    // Check a few values
    TS_ASSERT_EQUALS(output->readX(0)[0], in1->readX(0)[0]);
    TS_ASSERT_EQUALS(output->readX(15)[444], in2->readX(5)[444]);
    TS_ASSERT_EQUALS(output->readX(30)[444], in2->readX(5)[444]);
    TS_ASSERT_EQUALS(output->readY(3)[99], in1->readY(3)[99]);
    TS_ASSERT_EQUALS(output->readE(7)[700], in1->readE(7)[700]);
    TS_ASSERT_EQUALS(output->readY(19)[55], in2->readY(9)[55]);
    TS_ASSERT_EQUALS(output->readE(10)[321], in2->readE(0)[321]);
    TS_ASSERT_EQUALS(output->readY(34)[55], in2->readY(9)[55]);
    TS_ASSERT_EQUALS(output->readE(25)[321], in2->readE(0)[321]);
    // There will be a spectra number clash here so all spectra numbers
    // should be reset
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(5), 5);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(12), 12);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(27), 27);
  }

  //----------------------------------------------------------------------------------------------
  void testExecMismatchedWorkspaces() {
    MatrixWorkspace_sptr ews =
        WorkspaceCreationHelper::createEventWorkspace(10, 10);

    // Check it fails if mixing event workspaces and workspace 2Ds
    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace1", ews));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace2", WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "outevent"));
    alg.execute();
    TS_ASSERT(!alg.isExecuted());
  }

  void testExecNonConstantBins() {
    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace1", WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace2", WorkspaceCreationHelper::create2DWorkspace(10, 15)));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "outExecNonConstantBins"));
    alg.execute();
    TS_ASSERT(!alg.isExecuted());
  }

  //----------------------------------------------------------------------------------------------
  void doTest(bool event, bool combineLogs = false) {
    MatrixWorkspace_sptr ws1, ws2, out;
    int numBins = 20;

    if (event) {
      ws1 = WorkspaceCreationHelper::createEventWorkspace2(
          10, numBins); // 2 events per bin
      ws2 = WorkspaceCreationHelper::createEventWorkspace2(5, numBins);
    } else {
      ws1 = WorkspaceCreationHelper::create2DWorkspace(10, numBins);
      ws2 = WorkspaceCreationHelper::create2DWorkspace(5, numBins);
    }
    // Add instrument so detector IDs are valid and get copied.
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws1, false, false,
                                                           "");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws2, false, false,
                                                           "");

    auto ws1Log = new TimeSeriesProperty<std::string>("aLog");
    ws1Log->addValue(DateAndTime("2014-06-19T16:40:00"), "Hello");
    ws1->mutableRun().addLogData(ws1Log);

    auto ws2Log = new TimeSeriesProperty<std::string>("aLog");
    ws2Log->addValue(DateAndTime("2014-06-19T16:40:10"), "World");
    ws2->mutableRun().addLogData(ws2Log);

    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    AppendSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace1", ws1Name));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace2", ws2Name));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", ws1Name));
    if (combineLogs)
      alg.setProperty("MergeLogs", true);
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        out = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(ws1Name));)
    TS_ASSERT(out);
    if (!out)
      return;

    TS_ASSERT_EQUALS(out->getNumberHistograms(), 15);
    TS_ASSERT_EQUALS(out->blocksize(), numBins);

    for (size_t wi = 0; wi < out->getNumberHistograms(); wi++) {
      TS_ASSERT_EQUALS(out->getSpectrum(wi).getSpectrumNo(), specnum_t(wi));
      TS_ASSERT(!out->getSpectrum(wi).getDetectorIDs().empty());
      const auto &y = out->y(wi);
      for (const auto value : y)
        TS_ASSERT_DELTA(value, 2.0, 1e-5);
    }

    if (combineLogs) {
      TS_ASSERT_EQUALS(
          out->run().getTimeSeriesProperty<std::string>("aLog")->size(), 2)
    } else {
      TS_ASSERT_EQUALS(
          out->run().getTimeSeriesProperty<std::string>("aLog")->size(), 1)
    }
  }

  void test_events() { doTest(true); }

  void test_2D() { doTest(false); }

  void test_events_mergeLogs() { doTest(true, true); }

  void test_2D_mergeLogs() { doTest(false, true); }

  void test_notEmptyTextAxis() {
    const std::string inputWorkspace = "weRebinned";
    const std::string outputWorkspace = "appended";

    createWorkspaceWithAxisAndLabel(inputWorkspace, "Text", "Text");
    doTestAppendSpectraWithWorkspaces(inputWorkspace, inputWorkspace,
                                      outputWorkspace);
    MatrixWorkspace_const_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace);
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis = inputWS->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis->label(0), outputAxis->label(i));
    }
  }

  void test_emptyTextAxis() {
    const std::string inputWorkspace = "weRebinned";
    const std::string outputWorkspace = "appended";

    createWorkspaceWithAxisAndLabel(inputWorkspace, "Text", "");
    doTestAppendSpectraWithWorkspaces(inputWorkspace, inputWorkspace,
                                      outputWorkspace);
    MatrixWorkspace_const_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace);
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis = inputWS->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis->label(0), outputAxis->label(i));
    }
  }

  void test_emptyAndNotEmptyTextAxis() {
    const std::string inputWorkspace1 = "weRebinned1";
    const std::string inputWorkspace2 = "weRebinned2";
    const std::string outputWorkspace = "appended";

    createWorkspaceWithAxisAndLabel(inputWorkspace1, "Text", "Text");
    createWorkspaceWithAxisAndLabel(inputWorkspace2, "Text", "");
    doTestAppendSpectraWithWorkspaces(inputWorkspace1, inputWorkspace2,
                                      outputWorkspace);

    MatrixWorkspace_const_sptr inputWS1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace1);
    MatrixWorkspace_const_sptr inputWS2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace2);
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis1 = inputWS1->getAxis(1);
    const auto inputAxis2 = inputWS2->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    const auto ws1len = inputWS1->getNumberHistograms();

    for (size_t i = 0; i < outputWS->getNumberHistograms() / 2; ++i) {
      // check that all labels are the same
      // this axis label will have value "Text"
      TS_ASSERT_EQUALS(inputAxis1->label(0), outputAxis->label(i));

      // this axis label will have value "" <= empty string
      // this will check the labels for the second workspace that is
      // appended at position starting from the length of the first workspace
      TS_ASSERT_EQUALS(inputAxis2->label(0), outputAxis->label(i + ws1len));
    }
  }

  void test_numericAxis() {
    const std::string inputWorkspace = "weRebinned";
    const std::string outputWorkspace = "appended";

    createWorkspaceWithAxisAndLabel(inputWorkspace, "Time", "1.0");
    doTestAppendSpectraWithWorkspaces(inputWorkspace, inputWorkspace,
                                      outputWorkspace);
    MatrixWorkspace_const_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace);
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis = inputWS->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis->getValue(0), outputAxis->getValue(i));
    }
  }

  void test_differentNumericAxis() {
    const std::string inputWorkspace1 = "weRebinned1";
    const std::string inputWorkspace2 = "weRebinned2";
    const std::string outputWorkspace = "appended";

    createWorkspaceWithAxisAndLabel(inputWorkspace1, "Time", "1.0");
    createWorkspaceWithAxisAndLabel(inputWorkspace2, "Time", "2.0");
    doTestAppendSpectraWithWorkspaces(inputWorkspace1, inputWorkspace2,
                                      outputWorkspace);

    MatrixWorkspace_const_sptr inputWS1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace1);
    MatrixWorkspace_const_sptr inputWS2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWorkspace2);
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWorkspace);

    // Y axis number is 1, no need to cast up to TextAxis as we're only reading
    // the values
    const auto inputAxis1 = inputWS1->getAxis(1);
    const auto inputAxis2 = inputWS2->getAxis(1);
    const auto outputAxis = outputWS->getAxis(1);

    const auto ws1len = inputWS1->getNumberHistograms();

    for (size_t i = 0; i < outputWS->getNumberHistograms() / 2; ++i) {
      // check that all labels are the same
      TS_ASSERT_EQUALS(inputAxis1->getValue(0), outputAxis->getValue(i));
      // this will check the labels for the second workspace that is
      // appended at position starting from the length of the first workspace
      TS_ASSERT_EQUALS(inputAxis2->getValue(0),
                       outputAxis->getValue(i + ws1len));
    }
  }

private:
  void doTestAppendSpectraWithWorkspaces(const std::string &inputWorkspace1,
                                         const std::string &inputWorkspace2,
                                         const std::string &outputWorkspace) {
    auto appendSpectra =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "AppendSpectra");
    TS_ASSERT_THROWS_NOTHING(appendSpectra->setRethrows(true));
    TS_ASSERT_THROWS_NOTHING(
        appendSpectra->setProperty("InputWorkspace1", inputWorkspace1));
    TS_ASSERT_THROWS_NOTHING(
        appendSpectra->setProperty("InputWorkspace2", inputWorkspace2));
    TS_ASSERT_THROWS_NOTHING(
        appendSpectra->setProperty("OutputWorkspace", outputWorkspace));
    TS_ASSERT_THROWS_NOTHING(appendSpectra->execute());
    TS_ASSERT(appendSpectra->isExecuted());
  }
  /** Creates a 2D workspace with 5 histograms
   */
  void createWorkspaceWithAxisAndLabel(const std::string outputName,
                                       const std::string &axisType,
                                       const std::string axisValue) {
    int nspec = 5;
    std::vector<std::string> YVals;
    std::vector<double> dataX;
    std::vector<double> dataY;

    for (auto i = 0; i < nspec; ++i) {
      YVals.push_back(axisValue);
    }

    for (int i = 0; i < 100; ++i) {
      dataX.push_back(double(i));
      dataY.push_back(double(i));
    }

    auto createWS = Mantid::API::FrameworkManager::Instance().createAlgorithm(
        "CreateWorkspace");
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("OutputWorkspace", "we"));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("DataX", dataX));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("DataY", dataY));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("NSpec", nspec));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("UnitX", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(
        createWS->setProperty("VerticalAxisUnit", axisType));
    TS_ASSERT_THROWS_NOTHING(
        createWS->setProperty("VerticalAxisValues", YVals));
    TS_ASSERT_THROWS_NOTHING(createWS->setProperty("YUnitLabel", "Counts"));
    TS_ASSERT_THROWS_NOTHING(createWS->execute());

    // we do a rebin so we can have nice bins
    auto rebin =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("Rebin");
    TS_ASSERT_THROWS_NOTHING(rebin->setProperty("InputWorkspace", "we"));
    TS_ASSERT_THROWS_NOTHING(
        rebin->setProperty("Params", std::vector<double>{1}));
    TS_ASSERT_THROWS_NOTHING(rebin->setProperty("OutputWorkspace", outputName));
    TS_ASSERT_THROWS_NOTHING(rebin->execute());
    TS_ASSERT(rebin->isExecuted());
  }

  const std::string ws1Name;
  const std::string ws2Name;
};

#endif /* MANTID_ALGORITHMS_APPENDSPECTRATEST_H_ */
