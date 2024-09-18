// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadCanSAS1D2.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/SaveCanSAS1D2.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <fstream>
#include <sstream>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class SaveCanSAS1dTest2 : public CxxTest::TestSuite {
public:
  static SaveCanSAS1dTest2 *createSuite() { return new SaveCanSAS1dTest2(); }
  static void destroySuite(SaveCanSAS1dTest2 *suite) { delete suite; }

  // set up the workspace that will be loaded
  SaveCanSAS1dTest2()
      : m_workspace1("SaveCanSAS1dTest2_in1"), m_workspace2("SaveCanSAS1dTest2_in2"),
        m_workspace3("SaveCanSAS1dTest2_in3"), m_workspace4("SaveCanSAS1dTest2_in4"), m_filename("./savecansas1d2.xml")

  {
    LoadRaw3 loader;
    if (!loader.isInitialized())
      loader.initialize();
    std::string inputFile; // Path to test input file assumes Test directory
                           // checked out from SVN

    // the file's run number needs to be stored in m_runNum for later tests
    inputFile = "LOQ48127.raw";
    m_runNum = "48127";

    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("OutputWorkspace", m_workspace1);
    loader.setPropertyValue("SpectrumList", "1");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Change the unit to Q
    ws = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_workspace1));
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    {
      LoadRaw3 loader;
      if (!loader.isInitialized())
        loader.initialize();
      // simulate loading the transmission
      loader.setPropertyValue("Filename", inputFile);
      loader.setPropertyValue("OutputWorkspace", m_workspace3);
      loader.setPropertyValue("SpectrumList", "1");
      TS_ASSERT_THROWS_NOTHING(loader.execute());
      TS_ASSERT(loader.isExecuted());

      // Change the unit to Wavelength
      ws = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_workspace3));
      ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    }

    WorkspaceGroup_sptr group(new WorkspaceGroup);
    AnalysisDataService::Instance().addOrReplace("SaveCanSAS1dTest2_group", group);

    group->add(m_workspace1);

    LoadRaw3 load;
    load.initialize();
    load.setPropertyValue("Filename", "IRS26173.raw");
    load.setPropertyValue("OutputWorkspace", m_workspace2);
    load.setPropertyValue("SpectrumList", "30");
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT(load.isExecuted());

    // Change the unit to Q
    ws = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_workspace2));
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    group->add(m_workspace2);

    auto ws4 = WorkspaceCreationHelper::create2DWorkspace(3, 5);
    ws4->getAxis(0)->setUnit("MomentumTransfer");
    AnalysisDataService::Instance().addOrReplace("SaveCanSAS1dTest2_in4", ws4);
  }

  // saving is required by all the following test so, if this test fails so will
  // all the others!
  void setUp() override {
    SaveCanSAS1D2 savealg;

    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    TS_ASSERT(savealg.isInitialized());
    savealg.setPropertyValue("InputWorkspace", m_workspace1);
    savealg.setPropertyValue("Filename", m_filename);
    savealg.setPropertyValue("Transmission", m_workspace3);
    savealg.setPropertyValue("DetectorNames", "HAB");
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT(savealg.isExecuted());
    // Get the full path to the file again
    m_filename = savealg.getPropertyValue("Filename");
  }

  void tearDown() override {
    if (Poco::File(m_filename).exists())
      Poco::File(m_filename).remove();
  }

  void testCanSAS1dXML() {
    // read the generated xml file and compare first few lines of the file
    std::ifstream testFile(m_filename.c_str(), std::ios::in);
    TS_ASSERT(testFile);
    if (!testFile)
      return;

    // testing the first few lines of the xml file
    std::string fileLine;
    std::getline(testFile, fileLine);
    std::getline(testFile, fileLine);

    std::getline(testFile, fileLine);
    std::string sasRootexpected = fileLine;

    std::getline(testFile, fileLine);
    sasRootexpected += fileLine;
    std::getline(testFile, fileLine);
    sasRootexpected += fileLine;

    std::getline(testFile, fileLine);
    sasRootexpected += fileLine;

    std::getline(testFile, fileLine);
    sasRootexpected += fileLine;

    std::string sasroot;
    sasroot = "<SASroot version=\"1.1\"";
    sasroot += "\t\t xmlns=\"urn:cansas1d:1.1\"";
    sasroot += "\t\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
    sasroot += "\t\t xsi:schemaLocation=\"urn:cansas1d:1.1 "
               "http://www.cansas.org/formats/1.1/cansas1d.xsd\"";
    sasroot += "\t\t>";
    TS_ASSERT_EQUALS(sasroot, sasRootexpected);

    std::getline(testFile, fileLine);
    {
      std::ostringstream correctLine;
      correctLine << "\t<SASentry name=\"" << m_workspace1 << "\">";
      TS_ASSERT_EQUALS(fileLine, correctLine.str());
    }

    std::getline(testFile, fileLine);
    TS_ASSERT_EQUALS(fileLine, "\t\t<Title>direct beam                         "
                               "                                            "
                               "</Title>");
    std::getline(testFile, fileLine);
    {
      std::ostringstream correctLine;
      correctLine << "\t\t<Run>" << m_runNum << "</Run>";
      TS_ASSERT_EQUALS(fileLine, correctLine.str());
    }

    std::getline(testFile, fileLine);
    TS_ASSERT_EQUALS(fileLine, "\t\t<SASdata>");

    std::getline(testFile, fileLine);
    std::string idataline = "\t\t\t<Idata><Q unit=\"1/A\">3543.75</Q><I "
                            "unit=\"Counts\">111430</I><Idev "
                            "unit=\"Counts\">333.811</Idev><Qdev "
                            "unit=\"1/A\">0</Qdev></Idata>";
    TS_ASSERT_EQUALS(fileLine, idataline);

    for (int i = 0; i < 101; i++) {
      std::getline(testFile, fileLine); // should read all the SASData
    }
    std::getline(testFile, fileLine);
    TS_ASSERT_EQUALS(fileLine, "\t\t</SASdata>");

    std::getline(testFile, fileLine); // transmission spectrum start
    TS_ASSERT_EQUALS(fileLine, "\t\t<SAStransmission_spectrum name=\"sample\">");

    idataline = "\t\t\t<Tdata><Lambda unit=\"A\">3543.75</Lambda><T "
                "unit=\"Counts\">111430</T><Tdev "
                "unit=\"none\">333.811</Tdev></Tdata>";
    std::getline(testFile, fileLine); // transmission spectrum data
    TS_ASSERT_EQUALS(fileLine, idataline);

    for (int i = 0; i < 101; i++) {
      std::getline(testFile, fileLine); // should read all the spectrum data
    }

    for (int i = 0; i < 10; i++) {
      std::getline(testFile,
                   fileLine); // should read some of sample information
    }

    std::getline(testFile, fileLine); // should read SASDETECTOR
    std::getline(testFile, fileLine); // should read name

    TS_ASSERT_EQUALS(fileLine, "\t\t\t\t<name>HAB</name>");

    testFile.close();

    // no more tests on the file are possible after this
    if (Poco::File(m_filename).exists())
      Poco::File(m_filename).remove();
  }

  void testCanSetAdditionalRunNumbersAsProperties() {
    // Initialize alg
    SaveCanSAS1D2 savealg;

    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    TS_ASSERT(savealg.isInitialized());
    savealg.setPropertyValue("InputWorkspace", m_workspace1);
    savealg.setPropertyValue("Filename", m_filename);
    savealg.setPropertyValue("DetectorNames", "HAB");

    // Set the additional run number properties
    TSM_ASSERT_THROWS_NOTHING("Should be able to set SampleTransmissionRunNumber property",
                              savealg.setProperty("SampleTransmissionRunNumber", "5"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set SampleDirectRunNumber property",
                              savealg.setProperty("SampleDirectRunNumber", "6"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set CanScatterRunNumber property",
                              savealg.setProperty("CanScatterRunNumber", "7"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set CanDirectRunNumber property",
                              savealg.setProperty("CanDirectRunNumber", "8"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT(savealg.isExecuted());
  }

  void testCanSetScaledBackgroundSubtractionMetadataAsProperties() {
    // Initialize alg
    SaveCanSAS1D2 savealg;

    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    TS_ASSERT(savealg.isInitialized());
    savealg.setPropertyValue("InputWorkspace", m_workspace1);
    savealg.setPropertyValue("Filename", m_filename);
    savealg.setPropertyValue("DetectorNames", "HAB");

    // Set the additional run number properties
    TSM_ASSERT_THROWS_NOTHING("Should be able to set BackgroundSubtractionWorkspace property",
                              savealg.setProperty("BackgroundSubtractionWorkspace", "a_workspace"));
    TSM_ASSERT_THROWS_NOTHING("Should be able to set BackgroundSubtractionScaleFactor property",
                              savealg.setProperty("BackgroundSubtractionScaleFactor", 1.5));

    // Execute
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT(savealg.isExecuted());
  }

  void testGroup() {
    // do the save, the results of which we'll test
    SaveCanSAS1D2 savealg;
    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    TS_ASSERT(savealg.isInitialized());
    savealg.setPropertyValue("InputWorkspace", "SaveCanSAS1dTest2_group");
    savealg.setPropertyValue("Filename", m_filename);
    savealg.setPropertyValue("DetectorNames", "HAB");
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT(savealg.isExecuted());

    // retrieve the data that we saved to check it
    LoadCanSAS1D lAlg;
    TS_ASSERT_THROWS_NOTHING(lAlg.initialize());
    TS_ASSERT(lAlg.isInitialized());
    lAlg.setPropertyValue("OutputWorkspace", "newgroup");
    lAlg.setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(lAlg.execute());
    TS_ASSERT(lAlg.isExecuted());
    Workspace_sptr ws = AnalysisDataService::Instance().retrieve("newgroup");
    WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(ws);

    // we have the data now the tests begin
    TS_ASSERT(group);
    if (!group)
      return; // To avoid breaking the rest of the test runner.
    std::vector<std::string> wNames = group->getNames();

    TS_ASSERT_EQUALS(wNames.size(),
                     2); // change this and the lines below when group workspace names change
    TS_ASSERT_EQUALS(wNames[0], m_workspace1);
    TS_ASSERT_EQUALS(wNames[1], m_workspace2);

    // check the second workspace in more detail
    ws = Mantid::API::AnalysisDataService::Instance().retrieve(wNames[1]);
    Mantid::DataObjects::Workspace2D_sptr ws2d = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);
    TS_ASSERT(ws2d);

    Run run = ws2d->run();
    Mantid::Kernel::Property *logP = run.getLogData("run_number");
    TS_ASSERT_EQUALS(logP->value(), "26173");
    TS_ASSERT_EQUALS(ws2d->getInstrument()->getName(), "IRIS");

    TS_ASSERT_EQUALS(ws2d->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws2d->x(0).size(), 2000);

    // some of the data is only stored to 3 decimal places
    double tolerance(1e-04);
    TS_ASSERT_DELTA(ws2d->x(0).front(), 56005, tolerance);
    TS_ASSERT_DELTA(ws2d->x(0)[1000], 66005, tolerance);
    TS_ASSERT_DELTA(ws2d->x(0).back(), 75995, tolerance);

    TS_ASSERT_DELTA(ws2d->y(0).front(), 0, tolerance);
    TS_ASSERT_DELTA(ws2d->y(0)[1000], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d->y(0).back(), 0, tolerance);

    TS_ASSERT_DELTA(ws2d->e(0).front(), 0, tolerance);
    TS_ASSERT_DELTA(ws2d->e(0)[1000], 1.0, tolerance);
    TS_ASSERT_DELTA(ws2d->e(0).back(), 0, tolerance);
  }

  void test_that_can_save_and_load_full_collimation_information() {
    std::string geometry = "Disc";
    double width = 1;
    double height = 2;
    int expectedGeometryFlag = 3;
    double expectedWidth = 1;
    double expectedHeight = 2;
    do_test_collimation_settings(geometry, width, height, expectedGeometryFlag, expectedWidth, expectedHeight);
  }

  void test_one_spectrum_per_file() {
    size_t extPos = m_filename.find(".xml");
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_workspace4);
    SaveCanSAS1D2 savealg;
    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    savealg.setPropertyValue("InputWorkspace", m_workspace4);
    savealg.setPropertyValue("Filename", m_filename);
    savealg.setProperty("OneSpectrumPerFile", true);

    // spectrum axis
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(m_filename, 0, extPos) << "_" << spec << std::string(m_filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }

    // numeric axis
    std::unique_ptr<Axis> numericAxis = std::make_unique<NumericAxis>(3);
    for (int i = 0; i < 3; ++i) {
      numericAxis->setValue(i, i * i);
    }
    ws->replaceAxis(1, std::move(numericAxis));
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(m_filename, 0, extPos) << "_" << spec << "_" << spec * spec << std::string(m_filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }

    // bin edge axis
    std::unique_ptr<Axis> binEdgeAxis = std::make_unique<BinEdgeAxis>(4);
    for (int i = 0; i < 4; ++i) {
      binEdgeAxis->setValue(i, i * i);
    }
    ws->replaceAxis(1, std::move(binEdgeAxis));
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(m_filename, 0, extPos) << "_" << spec << "_" << 0.5 * (spec * spec + (spec + 1) * (spec + 1))
         << std::string(m_filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }

    // text axis
    std::unique_ptr<TextAxis> textAxis = std::make_unique<TextAxis>(3);
    for (int i = 0; i < 3; ++i) {
      textAxis->setLabel(i, std::string("ax_") + std::to_string(i));
    }
    ws->replaceAxis(1, std::unique_ptr<Axis>(std::move(textAxis)));
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(m_filename, 0, extPos) << "_" << spec << "_ax_" << spec << std::string(m_filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }
  }

private:
  void do_test_collimation_settings(const std::string &geometry, double width, double height, int expectedGeometry,
                                    double expectedWidth, double expectedHeight) {
    // Create sample workspace
    auto wsIn = WorkspaceCreationHelper::create1DWorkspaceRand(3, true);
    auto axis = wsIn->getAxis(0);
    axis->unit() = UnitFactory::Instance().create("MomentumTransfer");
    axis->title() = "|Q|";

    AnalysisDataService::Instance().addOrReplace("test_worksapce_can_sas_1d", wsIn);
    // Save the workspace
    SaveCanSAS1D2 savealg;
    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    TS_ASSERT(savealg.isInitialized());
    savealg.setProperty("InputWorkspace", wsIn);
    savealg.setPropertyValue("Filename", m_filename);
    savealg.setPropertyValue("DetectorNames", "HAB");
    savealg.setProperty("Geometry", geometry);
    savealg.setProperty("SampleWidth", width);
    savealg.setProperty("SampleHeight", height);

    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    TS_ASSERT(savealg.isExecuted());

    // retrieve the data that we saved to check it
    LoadCanSAS1D lAlg;
    TS_ASSERT_THROWS_NOTHING(lAlg.initialize());
    TS_ASSERT(lAlg.isInitialized());
    lAlg.setPropertyValue("OutputWorkspace", "test_worksapce_can_sas_1d_reloaded");
    lAlg.setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(lAlg.execute());
    TS_ASSERT(lAlg.isExecuted());
    Workspace_sptr ws = AnalysisDataService::Instance().retrieve("test_worksapce_can_sas_1d_reloaded");
    Mantid::API::MatrixWorkspace_sptr loaded = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);

    // Check that elements are set correctly
    TS_ASSERT(loaded->sample().getGeometryFlag() == expectedGeometry);
    TS_ASSERT(loaded->sample().getWidth() == expectedWidth);
    TS_ASSERT(loaded->sample().getHeight() == expectedHeight);

    // Delete workspaces
    std::string toDeleteList[2] = {"test_worksapce_can_sas_1d", "test_worksapce_can_sas_1d_reloaded"};
    for (auto &toDelete : toDeleteList) {
      if (AnalysisDataService::Instance().doesExist(toDelete)) {
        AnalysisDataService::Instance().remove(toDelete);
      }
    }
  }

  std::string m_workspace1, m_workspace2, m_workspace3, m_workspace4, m_filename;
  std::string m_runNum;
  MatrixWorkspace_sptr ws;
};
