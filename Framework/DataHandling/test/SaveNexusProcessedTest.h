// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NeXusFile.hpp"
#include <Poco/File.h>
#include <Poco/Path.h>

#include <boost/lexical_cast.hpp>

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <memory>

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.hxx"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::HistogramData::HistogramDx;

class SaveNexusProcessedTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusProcessedTest *createSuite() { return new SaveNexusProcessedTest(); }
  static void destroySuite(SaveNexusProcessedTest *suite) { delete suite; }

  SaveNexusProcessedTest() {
    // clearfiles - make true for SVN as dont want to leave on build server.
    // Unless the file "KEEP_NXS_FILES" exists, then clear up nxs files
    Poco::File file("KEEP_NXS_FILES");
    clearfiles = !file.exists();
  }

  void setUp() override {}

  void tearDown() override {}

  void testInit() {
    SaveNexusProcessed algToBeTested;
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExec() {
    auto useXErrors = false;
    std::string outputFile = "SaveNexusProcessedTest_testExec.nxs";
    outputFile = doExec(outputFile, useXErrors);

    // Clean up
    if (clearfiles)
      Poco::File(outputFile).remove();
    AnalysisDataService::Instance().remove("testSpace");
  }

  void testExecWithXErrors() {
    auto useXErrors = true;
    std::string outputFile = "SaveNexusProcessedTest_testExec.nxs";
    outputFile = doExec(outputFile, useXErrors);

    // Assert XError correctness
    ::NeXus::File savedNexus(outputFile);
    savedNexus.openGroup("mantid_workspace_1", "NXentry");
    savedNexus.openGroup("workspace", "NXdata");

    TSM_ASSERT_THROWS_NOTHING("Should find xerrors entry", savedNexus.openData("xerrors"));
    savedNexus.close();
    // Clean up
    if (clearfiles)
      Poco::File(outputFile).remove();
    AnalysisDataService::Instance().remove("testSpace");
  }

  void testExecOnLoadraw() {
    SaveNexusProcessed algToBeTested;
    std::string inputFile = "LOQ48127.raw";
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer4";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    //
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "SaveNexusProcessedTest_testExecOnLoadraw.nxs";
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
    dataName = "spectra";
    title = "A save of a workspace from Loadraw file";
    algToBeTested.setPropertyValue("Filename", outputFile);

    algToBeTested.setPropertyValue("Title", title);
    algToBeTested.setPropertyValue("Append", "0");
    outputFile = algToBeTested.getPropertyValue("Filename");
    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("Filename"));
    TS_ASSERT(!result.compare(outputFile));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    if (clearfiles)
      remove(outputFile.c_str());
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));
  }

  void testExecOnMuon() {
    SaveNexusProcessed algToBeTested;

    LoadNexus nxLoad;
    std::string outputSpace, inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("Filename", inputFile);
    outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "SaveNexusProcessedTest_testExecOnMuon.nxs";
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
    dataName = "spectra";
    title = "A save of a 2D workspace from Muon file";
    algToBeTested.setPropertyValue("Filename", outputFile);
    outputFile = algToBeTested.getPropertyValue("Filename");
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();

    algToBeTested.setPropertyValue("Title", title);
    algToBeTested.setPropertyValue("Append", "0");

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("Filename"));
    TS_ASSERT(!result.compare(outputFile));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    if (clearfiles)
      Poco::File(outputFile).remove();
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(outputSpace));
  }

  /**
   *
   * @param filename_root :: base of the file to save
   * @param type :: event type to create
   * @param outputFile[out] :: returns the output file
   * @param makeDifferentTypes :: mix event types
   * @param clearfiles :: clear files after saving
   * @param PreserveEvents :: save as event list
   * @param CompressNexus :: compress
   * @return
   */
  static EventWorkspace_sptr do_testExec_EventWorkspaces(const std::string &filename_root, EventType type,
                                                         std::string &outputFile, bool makeDifferentTypes,
                                                         bool clearfiles, bool PreserveEvents = true,
                                                         bool CompressNexus = false) {
    std::vector<std::vector<int>> groups(5);
    groups[0].emplace_back(10);
    groups[0].emplace_back(11);
    groups[0].emplace_back(12);
    groups[1].emplace_back(20);
    groups[2].emplace_back(30);
    groups[2].emplace_back(31);
    groups[3].emplace_back(40);
    groups[4].emplace_back(50);

    EventWorkspace_sptr WS = WorkspaceCreationHelper::createGroupedEventWorkspace(groups, 100, 1.0, 1.0);
    WS->getSpectrum(3).clear(false);
    // Switch the event type
    if (makeDifferentTypes) {
      WS->getSpectrum(0).switchTo(TOF);
      WS->getSpectrum(1).switchTo(WEIGHTED);
      WS->getSpectrum(2).switchTo(WEIGHTED_NOTIME);
      WS->getSpectrum(4).switchTo(WEIGHTED);
    } else {
      for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++)
        WS->getSpectrum(wi).switchTo(type);
    }

    SaveNexusProcessed alg;
    alg.initialize();

    // Now set it...
    alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(WS));

    // specify name of file to save workspace to
    std::ostringstream mess;
    mess << filename_root << static_cast<int>(type) << ".nxs";
    outputFile = mess.str();
    std::string dataName = "spectra";
    std::string title = "A simple workspace saved in Processed Nexus format";

    alg.setPropertyValue("Filename", outputFile);
    outputFile = alg.getPropertyValue("Filename");
    alg.setPropertyValue("Title", title);
    alg.setProperty("PreserveEvents", PreserveEvents);
    alg.setProperty("CompressNexus", CompressNexus);

    // Clear the existing file, if any
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(Poco::File(outputFile).exists());

    if (clearfiles)
      Poco::File(outputFile).remove();

    return WS;
  }

  void testExec_EventWorkspace_TofEvent() {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_", TOF, outputFile, false, clearfiles);
  }

  void testExec_EventWorkspace_WeightedEvent() {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_", WEIGHTED, outputFile, false, clearfiles);
  }

  void testExec_EventWorkspace_WeightedEventNoTime() {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_", WEIGHTED_NOTIME, outputFile, false, clearfiles);
  }

  void testExec_EventWorkspace_DifferentTypes() {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_DifferentTypes_", WEIGHTED_NOTIME, outputFile, true, clearfiles);
  }

  void testExec_EventWorkspace_DontPreserveEvents() {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_EventTo2D", TOF, outputFile, false, clearfiles,
                                false /* DONT preserve events */);
  }
  void testExec_EventWorkspace_CompressNexus() {
    std::string outputFile;
    do_testExec_EventWorkspaces("SaveNexusProcessed_EventTo2D", TOF, outputFile, false, clearfiles,
                                true /* DONT preserve events */, true /* Compress */);
  }

  void testExecSaveLabel() {
    SaveNexusProcessed alg;
    if (!alg.isInitialized())
      alg.initialize();

    // create dummy 2D-workspace
    Workspace2D_sptr localWorkspace2D =
        std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));

    // set units to be a label
    localWorkspace2D->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
    auto label = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(localWorkspace2D->getAxis(0)->unit());
    label->setLabel("Temperature", "K");

    double d = 0.0;
    for (int i = 0; i < 10; ++i, d += 0.1) {
      localWorkspace2D->dataX(0)[i] = d;
      localWorkspace2D->dataY(0)[i] = d;
      localWorkspace2D->dataE(0)[i] = d;
    }

    AnalysisDataService::Instance().addOrReplace("testSpace", localWorkspace2D);

    // Now set it...
    // specify name of file to save workspace to
    alg.setPropertyValue("InputWorkspace", "testSpace");
    outputFile = "SaveNexusProcessedTest_testExec.nxs";
    dataName = "spectra";
    title = "A simple workspace saved in Processed Nexus format";
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outputFile));
    outputFile = alg.getPropertyValue("Filename");
    alg.setPropertyValue("Title", title);
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = alg.getPropertyValue("Filename"));
    TS_ASSERT(!result.compare(outputFile));

    // changed so that 1D workspaces are no longer written.
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (clearfiles)
      Poco::File(outputFile).remove();

    AnalysisDataService::Instance().remove("testSpace");
  }

  void testSaveGroupWorkspace() {
    const std::string output_filename = "SaveNexusProcessedTest_GroupWorkspaceFile.nxs";

    // Clean out any previous instances.
    bool doesFileExist = Poco::File(output_filename).exists();
    if (doesFileExist) {
      Poco::File(output_filename).remove();
    }
    const int nEntries = 3;
    const int nHist = 1;
    const int nBins = 1;
    const std::string stem = "test_group_ws";
    Mantid::API::WorkspaceGroup_sptr group_ws =
        WorkspaceCreationHelper::createWorkspaceGroup(nEntries, nHist, nBins, stem);

    SaveNexusProcessed alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();

    alg.setProperty("Filename", output_filename);
    alg.setProperty("InputWorkspace", group_ws);
    alg.execute();

    doesFileExist = Poco::File(output_filename).exists();
    TSM_ASSERT("File should have been created", doesFileExist);
    if (doesFileExist) {
      Poco::File(output_filename).remove();
    }
  }

  void testSaveTableVectorColumn() {
    std::string outputFileName = "SaveNexusProcessedTest_testSaveTableVectorColumn.nxs";

    // Create a table which we will save
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("vector_int", "IntVectorColumn");
    table->addColumn("vector_double", "DoubleVectorColumn");

    std::vector<double> d1, d2, d3;
    d1.emplace_back(0.5);
    d2.emplace_back(1.0);
    d2.emplace_back(2.5);
    d3.emplace_back(4.0);

    // Add some rows of different sizes
    TableRow row1 = table->appendRow();
    row1 << Strings::parseRange("1") << d1;
    TableRow row2 = table->appendRow();
    row2 << Strings::parseRange("2,3") << d2;
    TableRow row3 = table->appendRow();
    row3 << Strings::parseRange("4,5,6,7") << d3;

    ScopedWorkspace inputWsEntry(table);

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputWsEntry.name());
    alg.setPropertyValue("Filename", outputFileName);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted())
      return; // Nothing to check

    // Get full output file path
    outputFileName = alg.getPropertyValue("Filename");

    try {
      ::NeXus::File savedNexus(outputFileName);

      savedNexus.openGroup("mantid_workspace_1", "NXentry");
      savedNexus.openGroup("table_workspace", "NXdata");

      // -- Checking int column -----

      savedNexus.openData("column_1");

      ::NeXus::Info columnInfo1 = savedNexus.getInfo();
      TS_ASSERT_EQUALS(columnInfo1.dims.size(), 2);
      TS_ASSERT_EQUALS(columnInfo1.dims[0], 3);
      TS_ASSERT_EQUALS(columnInfo1.dims[1], 4);
      TS_ASSERT_EQUALS(columnInfo1.type, NXnumtype::INT32);

      std::vector<int> data1;
      savedNexus.getData<int>(data1);

      TS_ASSERT_EQUALS(data1.size(), 12);
      TS_ASSERT_EQUALS(data1[0], 1);
      TS_ASSERT_EQUALS(data1[3], 0);
      TS_ASSERT_EQUALS(data1[5], 3);
      TS_ASSERT_EQUALS(data1[8], 4);
      TS_ASSERT_EQUALS(data1[11], 7);

      std::vector<::NeXus::AttrInfo> attrInfos1 = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS(attrInfos1.size(), 6);

      if (attrInfos1.size() == 6) {
        TS_ASSERT_EQUALS(attrInfos1[0].name, "row_size_0");
        TS_ASSERT_EQUALS(savedNexus.getAttr<int>(attrInfos1[0]), 1);

        TS_ASSERT_EQUALS(attrInfos1[2].name, "row_size_2");
        TS_ASSERT_EQUALS(savedNexus.getAttr<int>(attrInfos1[2]), 4);

        TS_ASSERT_EQUALS(attrInfos1[4].name, "interpret_as");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos1[4]), "");

        TS_ASSERT_EQUALS(attrInfos1[5].name, "name");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos1[5]), "IntVectorColumn");
      }

      // -- Checking double column -----

      savedNexus.openData("column_2");

      ::NeXus::Info columnInfo2 = savedNexus.getInfo();
      TS_ASSERT_EQUALS(columnInfo2.dims.size(), 2);
      TS_ASSERT_EQUALS(columnInfo2.dims[0], 3);
      TS_ASSERT_EQUALS(columnInfo2.dims[1], 2);
      TS_ASSERT_EQUALS(columnInfo2.type, NXnumtype::FLOAT64);

      std::vector<double> data2;
      savedNexus.getData<double>(data2);

      TS_ASSERT_EQUALS(data2.size(), 6);
      TS_ASSERT_EQUALS(data2[0], 0.5);
      TS_ASSERT_EQUALS(data2[3], 2.5);
      TS_ASSERT_EQUALS(data2[5], 0.0);

      std::vector<::NeXus::AttrInfo> attrInfos2 = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS(attrInfos2.size(), 6);

      if (attrInfos2.size() == 6) {
        TS_ASSERT_EQUALS(attrInfos2[0].name, "row_size_0");
        TS_ASSERT_EQUALS(savedNexus.getAttr<int>(attrInfos2[0]), 1);

        TS_ASSERT_EQUALS(attrInfos2[1].name, "row_size_1");
        TS_ASSERT_EQUALS(savedNexus.getAttr<int>(attrInfos2[1]), 2);

        TS_ASSERT_EQUALS(attrInfos2[4].name, "interpret_as");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos2[4]), "");

        TS_ASSERT_EQUALS(attrInfos2[5].name, "name");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos2[5]), "DoubleVectorColumn");
      }
    } catch (std::exception &e) {
      TS_FAIL(e.what());
    }

    Poco::File(outputFileName).remove();
  }

  void testSaveTableColumn() {
    std::string outputFileName = "SaveNexusProcessedTest_testSaveTable.nxs";

    // Create a table which we will save
    auto table =
        std::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(WorkspaceFactory::Instance().createTable());
    table->setRowCount(3);
    table->addColumn("int", "IntColumn");
    {
      auto &data = table->getColVector<int>("IntColumn");
      data[0] = 5;
      data[1] = 2;
      data[2] = 3;
    }
    table->addColumn("double", "DoubleColumn");
    {
      auto &data = table->getColVector<double>("DoubleColumn");
      data[0] = 0.5;
      data[1] = 0.2;
      data[2] = 0.3;
    }
    table->addColumn("float", "FloatColumn");
    {
      auto &data = table->getColVector<float>("FloatColumn");
      data[0] = 10.5f;
      data[1] = 10.2f;
      data[2] = 10.3f;
    }
    table->addColumn("uint", "UInt32Column");
    {
      auto &data = table->getColVector<uint32_t>("UInt32Column");
      data[0] = 15;
      data[1] = 12;
      data[2] = 13;
    }
    table->addColumn("long64", "Int64Column");
    {
      auto &data = table->getColVector<int64_t>("Int64Column");
      data[0] = 25;
      data[1] = 22;
      data[2] = 23;
    }
    table->addColumn("size_t", "SizeColumn");
    {
      auto &data = table->getColVector<size_t>("SizeColumn");
      data[0] = 35;
      data[1] = 32;
      data[2] = 33;
    }
    table->addColumn("bool", "BoolColumn");
    {
      auto &data = table->getColVector<Boolean>("BoolColumn");
      data[0] = true;
      data[1] = false;
      data[2] = true;
    }
    table->addColumn("V3D", "V3DColumn");
    {
      auto &data = table->getColVector<V3D>("V3DColumn");
      data[0] = V3D(1, 2, 3);
      data[1] = V3D(4, 5, 6);
      data[2] = V3D(7, 8, 9);
    }
    table->addColumn("str", "StringColumn");
    {
      auto &data = table->getColVector<std::string>("StringColumn");
      data[0] = "First row";
      data[1] = "2";
      data[2] = "";
    }

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", table);
    alg.setPropertyValue("Filename", outputFileName);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted())
      return; // Nothing to check

    // Get full output file path
    outputFileName = alg.getPropertyValue("Filename");

    ::NeXus::File savedNexus(outputFileName);

    savedNexus.openGroup("mantid_workspace_1", "NXentry");
    savedNexus.openGroup("table_workspace", "NXdata");

    {
      savedNexus.openData("column_1");
      doTestColumnInfo(savedNexus, NXnumtype::INT32, "", "IntColumn");
      int32_t expectedData[] = {5, 2, 3};
      doTestColumnData("IntColumn", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_2");
      doTestColumnInfo(savedNexus, NXnumtype::FLOAT64, "", "DoubleColumn");
      double expectedData[] = {0.5, 0.2, 0.3};
      doTestColumnData("DoubleColumn", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_3");
      doTestColumnInfo(savedNexus, NXnumtype::FLOAT32, "", "FloatColumn");
      float expectedData[] = {10.5f, 10.2f, 10.3f};
      doTestColumnData("FloatColumn", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_4");
      doTestColumnInfo(savedNexus, NXnumtype::UINT32, "", "UInt32Column");
      uint32_t expectedData[] = {15, 12, 13};
      doTestColumnData("UInt32Column", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_5");
      doTestColumnInfo(savedNexus, NXnumtype::INT64, "", "Int64Column");
      int64_t expectedData[] = {25, 22, 23};
      doTestColumnData("Int64Column", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_6");
      doTestColumnInfo(savedNexus, NXnumtype::UINT64, "", "SizeColumn");
      uint64_t expectedData[] = {35, 32, 33};
      doTestColumnData("SizeColumn", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_7");
      doTestColumnInfo(savedNexus, NXnumtype::UINT8, "", "BoolColumn");
      unsigned char expectedData[] = {1, 0, 1};
      doTestColumnData("BoolColumn", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_8");
      doTestColumnInfo2(savedNexus, NXnumtype::FLOAT64, "V3D", "V3DColumn", 3);
      double expectedData[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
      doTestColumnData("V3DColumn", savedNexus, expectedData, 9);
    }

    {
      savedNexus.openData("column_9");

      ::NeXus::Info columnInfo = savedNexus.getInfo();
      TS_ASSERT_EQUALS(columnInfo.dims.size(), 2);
      TS_ASSERT_EQUALS(columnInfo.dims[0], 3);
      TS_ASSERT_EQUALS(columnInfo.dims[1], 9);
      TS_ASSERT_EQUALS(columnInfo.type, NXnumtype::CHAR);

      std::vector<::NeXus::AttrInfo> attrInfos = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS(attrInfos.size(), 3);

      if (attrInfos.size() == 3) {
        TS_ASSERT_EQUALS(attrInfos[1].name, "interpret_as");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos[1]), "A string");

        TS_ASSERT_EQUALS(attrInfos[2].name, "name");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos[2]), "StringColumn");

        TS_ASSERT_EQUALS(attrInfos[0].name, "units");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos[0]), "N/A");
      }

      std::vector<char> data;
      savedNexus.getData(data);
      TS_ASSERT_EQUALS(data.size(), 9 * 3);

      std::string first(data.begin(), data.begin() + 9);
      TS_ASSERT_EQUALS(first, "First row");

      std::string second(data.begin() + 9, data.begin() + 18);
      TS_ASSERT_EQUALS(second, "2        ");

      std::string third(data.begin() + 18, data.end());
      TS_ASSERT_EQUALS(third, "         ");
    }

    savedNexus.close();
    Poco::File(outputFileName).remove();
    AnalysisDataService::Instance().clear();
  }

  void testSaveTableEmptyColumn() {
    std::string outputFileName = "SaveNexusProcessedTest_testSaveTable.nxs";

    // Create a table which we will save
    auto table =
        std::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(WorkspaceFactory::Instance().createTable());
    table->setRowCount(3);
    table->addColumn("int", "IntColumn");
    {
      auto &data = table->getColVector<int>("IntColumn");
      data[0] = 5;
      data[1] = 2;
      data[2] = 3;
    }
    table->addColumn("str", "EmptyColumn");

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", table);
    alg.setPropertyValue("Filename", outputFileName);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted())
      return; // Nothing to check

    // Get full output file path
    outputFileName = alg.getPropertyValue("Filename");

    ::NeXus::File savedNexus(outputFileName);

    savedNexus.openGroup("mantid_workspace_1", "NXentry");
    savedNexus.openGroup("table_workspace", "NXdata");

    {
      savedNexus.openData("column_1");
      doTestColumnInfo(savedNexus, NXnumtype::INT32, "", "IntColumn");
      int32_t expectedData[] = {5, 2, 3};
      doTestColumnData("IntColumn", savedNexus, expectedData);
    }

    {
      savedNexus.openData("column_2");

      ::NeXus::Info columnInfo = savedNexus.getInfo();
      TS_ASSERT_EQUALS(columnInfo.dims.size(), 2);
      TS_ASSERT_EQUALS(columnInfo.dims[0], 3);
      TS_ASSERT_EQUALS(columnInfo.dims[1], 1);
      TS_ASSERT_EQUALS(columnInfo.type, NXnumtype::CHAR);

      std::vector<::NeXus::AttrInfo> attrInfos = savedNexus.getAttrInfos();
      TS_ASSERT_EQUALS(attrInfos.size(), 3);

      if (attrInfos.size() == 3) {
        TS_ASSERT_EQUALS(attrInfos[1].name, "interpret_as");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos[1]), "A string");

        TS_ASSERT_EQUALS(attrInfos[2].name, "name");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos[2]), "EmptyColumn");

        TS_ASSERT_EQUALS(attrInfos[0].name, "units");
        TS_ASSERT_EQUALS(savedNexus.getStrAttr(attrInfos[0]), "N/A");
      }

      std::vector<char> data;
      savedNexus.getData(data);
      TS_ASSERT_EQUALS(data.size(), 3);
      TS_ASSERT_EQUALS(data[0], ' ');
      TS_ASSERT_EQUALS(data[1], ' ');
      TS_ASSERT_EQUALS(data[2], ' ');
    }

    savedNexus.close();
    Poco::File(outputFileName).remove();
    AnalysisDataService::Instance().clear();
  }

  void test_masking() {
    LoadEmptyInstrument createWorkspace;
    createWorkspace.initialize();
    createWorkspace.setPropertyValue("Filename", "unit_testing/IDF_for_UNIT_TESTING.xml");
    createWorkspace.setPropertyValue("OutputWorkspace", "testSpace");
    createWorkspace.execute();
    auto ws = std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpace"));
    ws->mutableDetectorInfo().setMasked(1, true);
    TS_ASSERT_EQUALS(ws->detectorInfo().isMasked(0), false);
    TS_ASSERT_EQUALS(ws->detectorInfo().isMasked(1), true);
    TS_ASSERT_EQUALS(ws->detectorInfo().isMasked(2), false);

    SaveNexusProcessed saveAlg;
    saveAlg.initialize();
    saveAlg.setPropertyValue("InputWorkspace", "testSpace");
    std::string file = "SaveNexusProcessedTest_test_masking.nxs";
    if (Poco::File(file).exists())
      Poco::File(file).remove();
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("Filename", file));
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute());
    TS_ASSERT(saveAlg.isExecuted());

    LoadNexus loadAlg;
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", file);
    loadAlg.setPropertyValue("OutputWorkspace", "testSpaceReloaded");
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute());
    TS_ASSERT(loadAlg.isExecuted());
    auto wsReloaded =
        std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpaceReloaded"));
    TS_ASSERT_EQUALS(wsReloaded->detectorInfo().isMasked(0), false);
    TS_ASSERT_EQUALS(wsReloaded->detectorInfo().isMasked(1), true);
    TS_ASSERT_EQUALS(wsReloaded->detectorInfo().isMasked(2), false);

    if (clearfiles)
      Poco::File(file).remove();
    AnalysisDataService::Instance().remove("testSpace");
  }

  void test_ragged_x_bins_saves_correct_x_values_when_spectrum_indices_passed() {
    // stop regression related to bug in github issue #33152
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 2, 2);
    // alter binning of 1st spectrum
    ws->setX(0, make_cow<Mantid::HistogramData::HistogramX>(std::vector<double>{0.0, 2.0, 4.0}));
    AnalysisDataService::Instance().add("testSpace", ws);

    SaveNexusProcessed saveAlg;
    saveAlg.initialize();
    saveAlg.setPropertyValue("InputWorkspace", "testSpace");
    std::string file = "SaveNexusProcessedTest_test_ragged_bins_spectrum_indices.nxs";
    if (Poco::File(file).exists())
      Poco::File(file).remove();
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("Filename", file));
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("WorkspaceIndexList", "1")); // 2nd spectrum
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute());
    TS_ASSERT(saveAlg.isExecuted());

    LoadNexus loadAlg;
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", file);
    loadAlg.setPropertyValue("OutputWorkspace", "testSpaceReloaded");
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute());
    TS_ASSERT(loadAlg.isExecuted());
    auto wsReloaded =
        std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpaceReloaded"));
    // check has saved x values from 2nd spectrum not 1st
    TS_ASSERT_EQUALS(wsReloaded->readX(0), ws->readX(1));

    if (clearfiles)
      Poco::File(file).remove();
    AnalysisDataService::Instance().remove("testSpace");
  }

  void test_ragged_x_bins_input_data_bounds() {
    // Fix SEGFAULT when writing ragged data: respect input vector bounds at `putSlab`.

    // Implementation note:
    //   The preliminary implementation separated this test into a "negative" test (producing the SEGFAULT),
    // and a "positive" test (not producing the SEGFAULT).
    // The negative test was then wrapped, using `Poco::SignalHandler` and the `poco_throw_on_signal` macro.
    // Unfortunately, the current `CTest` implementation bypasses these mechanisms (how?),
    // and treats any abnormal termination as a failed test.  For the moment, the negative test requires
    // a "by hand" treatment:
    //   that is, test the previous version of `Mantid` using this version of `SaveNexusProcessedTest`,
    // and verify that this test fails due to a SEGFAULT.

    // Create a ragged workspace with rapidly decreasing spectrum lengths.
    using Counts = Mantid::HistogramData::Counts;
    using CountStandardDeviations = Mantid::HistogramData::CountStandardDeviations;
    using Histogram = Mantid::HistogramData::Histogram;
    using Histogram_sptr = std::shared_ptr<Histogram>;
    std::function<Histogram_sptr(double, double, std::size_t)> spectrumFunc = [](double x_0, double x_1,
                                                                                 std::size_t N_x) -> Histogram_sptr {
      const double dx = (x_1 - x_0) / (double(N_x - 1));
      Histogram_sptr rval = std::make_shared<Histogram>(Histogram::XMode::Points, Histogram::YMode::Counts);
      rval->resize(N_x); // resizes x: Points
      rval->setCounts(Counts(N_x));
      rval->setCountStandardDeviations(CountStandardDeviations(N_x));

      auto &vx = rval->mutableX();
      auto &vy = rval->mutableY();
      auto &ve = rval->mutableE();
      double x = x_0;
      for (std::size_t n = 0; n < N_x; ++n, x += dx) {
        vx[n] = x;
        vy[n] = 2.0;
        ve[n] = M_SQRT2;
      }
      return rval;
    };

    const std::size_t PAGE_SIZE(4096);
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceFromFunctionAndArgsList(
        spectrumFunc, {{0.0, 25600.0, std::size_t(256 * PAGE_SIZE)},
                       {0.0, 12800.0, std::size_t(128 * PAGE_SIZE)},
                       {0.0, 6400.0, std::size_t(64 * PAGE_SIZE)},
                       {0.0, 3200.0, std::size_t(32 * PAGE_SIZE)},
                       {0.0, 1600.0, std::size_t(16 * PAGE_SIZE)},
                       {0.0, 800.0, std::size_t(8 * PAGE_SIZE)},
                       {0.0, 400.0, std::size_t(4 * PAGE_SIZE)},
                       {0.0, 200.0, std::size_t(2 * PAGE_SIZE)}});
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws, false, false, "test instrument");
    TS_ASSERT(ws->isRaggedWorkspace());
    AnalysisDataService::Instance().add("testSpace", ws);

    SaveNexusProcessed saveAlg;
    saveAlg.initialize();
    saveAlg.setPropertyValue("InputWorkspace", "testSpace");
    std::string fileName = "SaveNexusProcessedTest_test_ragged_bins_data_bounds.nxs";
    if (Poco::File(fileName).exists())
      Poco::File(fileName).remove();
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("Filename", fileName));

    // Verify that the current implementation doesn't produce a SEGFAULT.
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute());
    TS_ASSERT(saveAlg.isExecuted());

    // If the save successfully executed without producing a SEGFAULT, this test is complete.
    if (clearfiles)
      Poco::File(fileName).remove();
    AnalysisDataService::Instance().remove("testSpace");
  }

  void test_nexus_spectraDetectorMap() {
    NexusTestHelper th(true);
    th.createFile("MatrixWorkspaceTest.nxs");
    auto ws = makeWorkspaceWithDetectors(100, 50);
    std::vector<int> wsIndex;
    for (int i = 0; i < 100; i++) {
      // Give some funny numbers, so it is not the default
      ws->getSpectrum(size_t(i)).setSpectrumNo(i * 11);
      ws->getSpectrum(size_t(i)).setDetectorID(99 - i);
      wsIndex.emplace_back(i);
    }
    SaveNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.saveSpectraDetectorMapNexus(*ws, th.file.get(), wsIndex, ::NeXus::LZW);)
    TS_ASSERT_THROWS_NOTHING(th.file->openData("detector_index"))
    std::vector<int32_t> data;
    TS_ASSERT_THROWS_NOTHING(th.file->getData(data))
    TS_ASSERT_EQUALS(data.size(), 100)
    for (size_t i{0}; i < data.size(); ++i) {
      TS_ASSERT_EQUALS(data[i], i)
    }
    TS_ASSERT_THROWS_NOTHING(th.file->closeData())
    TS_ASSERT_THROWS_NOTHING(th.file->openData("detector_count"))
    TS_ASSERT_THROWS_NOTHING(th.file->getData(data))
    TS_ASSERT_EQUALS(data.size(), 100)
    for (const auto i : data) {
      TS_ASSERT_EQUALS(i, 1)
    }
    TS_ASSERT_THROWS_NOTHING(th.file->closeData())
    TS_ASSERT_THROWS_NOTHING(th.file->openData("detector_list"))
    TS_ASSERT_THROWS_NOTHING(th.file->getData(data))
    TS_ASSERT_EQUALS(data.size(), 100)
    for (size_t i{0}; i < data.size(); ++i) {
      TS_ASSERT_EQUALS(data[i], 99 - i)
    }
  }

  void test_nexus_spectrumNumbers() {
    NexusTestHelper th(true);
    th.createFile("MatrixWorkspaceTest.nxs");
    auto ws = makeWorkspaceWithDetectors(100, 50);
    std::vector<int> wsIndex;
    for (int i = 0; i < 100; i++) {
      // Give some funny numbers, so it is not the default
      ws->getSpectrum(size_t(i)).setSpectrumNo(i * 11);
      wsIndex.emplace_back(i);
    }
    SaveNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.saveSpectrumNumbersNexus(*ws, th.file.get(), wsIndex, ::NeXus::LZW);)
    TS_ASSERT_THROWS_NOTHING(th.file->openData("spectra"))
    std::vector<int32_t> data;
    TS_ASSERT_THROWS_NOTHING(th.file->getData(data))
    TS_ASSERT_EQUALS(data.size(), 100)
    for (size_t i{0}; i < data.size(); ++i) {
      TS_ASSERT_EQUALS(data[i], i * 11)
    }
  }

  void test_when_nested_workspaces_are_being_saved() {
    Workspace2D_sptr ws1 =
        std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
    Workspace2D_sptr ws2 =
        std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));

    Mantid::API::WorkspaceGroup_sptr gws1 = std::make_shared<WorkspaceGroup>();
    gws1->addWorkspace(ws1);
    gws1->addWorkspace(ws2);
    Mantid::API::WorkspaceGroup_sptr gws2 = std::make_shared<WorkspaceGroup>();
    gws2->addWorkspace(gws1);
    AnalysisDataService::Instance().addOrReplace("gws2", gws2);

    SaveNexusProcessed saveAlg;
    saveAlg.initialize();
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("InputWorkspace", "gws2"));
    std::string file = "namesdoesntmatterasitshouldntsaveanyway.nxs";
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("Filename", file));
    TS_ASSERT_THROWS(saveAlg.execute(), const std::runtime_error &);
    TS_ASSERT(!saveAlg.isExecuted());
  }

private:
  void doTestColumnInfo(::NeXus::File &file, NXnumtype type, const std::string &interpret_as, const std::string &name) {
    ::NeXus::Info columnInfo = file.getInfo();
    TSM_ASSERT_EQUALS(name, columnInfo.dims.size(), 1);
    TSM_ASSERT_EQUALS(name, columnInfo.dims[0], 3);
    TSM_ASSERT_EQUALS(name, columnInfo.type, type);

    std::vector<::NeXus::AttrInfo> attrInfos = file.getAttrInfos();
    TSM_ASSERT_EQUALS(name, attrInfos.size(), 3);

    if (attrInfos.size() == 3) {
      TSM_ASSERT_EQUALS(name, attrInfos[1].name, "interpret_as");
      TSM_ASSERT_EQUALS(name, file.getStrAttr(attrInfos[1]), interpret_as);

      TSM_ASSERT_EQUALS(name, attrInfos[2].name, "name");
      TSM_ASSERT_EQUALS(name, file.getStrAttr(attrInfos[2]), name);

      TSM_ASSERT_EQUALS(name, attrInfos[0].name, "units");
      TSM_ASSERT_EQUALS(name, file.getStrAttr(attrInfos[0]), "Not known");
    }
  }

  void doTestColumnInfo2(::NeXus::File &file, NXnumtype type, const std::string &interpret_as, const std::string &name,
                         int dim1) {
    ::NeXus::Info columnInfo = file.getInfo();
    TSM_ASSERT_EQUALS(name, columnInfo.dims.size(), 2);
    TSM_ASSERT_EQUALS(name, columnInfo.dims[0], 3);
    TSM_ASSERT_EQUALS(name, columnInfo.dims[1], dim1);
    TSM_ASSERT_EQUALS(name, columnInfo.type, type);

    std::vector<::NeXus::AttrInfo> attrInfos = file.getAttrInfos();
    TSM_ASSERT_EQUALS(name, attrInfos.size(), 6);

    if (attrInfos.size() == 6) {
      TSM_ASSERT_EQUALS(name, attrInfos[4].name, "interpret_as");
      TSM_ASSERT_EQUALS(name, file.getStrAttr(attrInfos[4]), interpret_as);

      TSM_ASSERT_EQUALS(name, attrInfos[5].name, "name");
      TSM_ASSERT_EQUALS(name, file.getStrAttr(attrInfos[5]), name);

      TSM_ASSERT_EQUALS(name, attrInfos[3].name, "units");
      TSM_ASSERT_EQUALS(name, file.getStrAttr(attrInfos[3]), "Not known");
    }
  }

  template <typename T>
  void doTestColumnData(const std::string &name, ::NeXus::File &file, const T expectedData[], size_t len = 3) {
    std::vector<T> data;
    file.getData(data);

    TSM_ASSERT_EQUALS(name, data.size(), len);
    for (size_t i = 0; i < len; ++i) {
      std::string mess = name + ", item #" + boost::lexical_cast<std::string>(i);
      TSM_ASSERT_EQUALS(mess, data[i], expectedData[i]);
    };
  }

  std::string doExec(std::string outputFile = "SaveNexusProcessedTest_testExec.nxs", bool useXErrors = false) {
    SaveNexusProcessed algToBeTested;
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // create dummy 2D-workspace
    Workspace2D_sptr localWorkspace2D =
        std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10));
    localWorkspace2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    double d = 0.0;
    if (useXErrors) {
      localWorkspace2D->setPointStandardDeviations(0, 10);
    }
    for (int i = 0; i < 10; ++i, d += 0.1) {
      localWorkspace2D->dataX(0)[i] = d;
      localWorkspace2D->dataY(0)[i] = d;
      localWorkspace2D->dataE(0)[i] = d;
      if (useXErrors) {
        localWorkspace2D->mutableDx(0)[i] = d;
      }
    }

    AnalysisDataService::Instance().addOrReplace("testSpace", localWorkspace2D);

    // Now set it...
    // specify name of file to save workspace to
    algToBeTested.setPropertyValue("InputWorkspace", "testSpace");
    dataName = "spectra";
    title = "A simple workspace saved in Processed Nexus format";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("Filename", outputFile));
    outputFile = algToBeTested.getPropertyValue("Filename");
    algToBeTested.setPropertyValue("Title", title);
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("Filename"));
    TS_ASSERT(!result.compare(outputFile));

    // changed so that 1D workspaces are no longer written.
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    return outputFile;
  }

  /** Create a workspace with numSpectra, with
   * each spectrum having one detector, at id = workspace index.
   * @param numSpectra
   * @return
   */
  std::shared_ptr<MatrixWorkspace> makeWorkspaceWithDetectors(size_t numSpectra, size_t numBins) const {
    std::shared_ptr<MatrixWorkspace> ws2 = std::make_shared<WorkspaceTester>();
    ws2->initialize(numSpectra, numBins, numBins);

    auto inst = std::make_shared<Instrument>("TestInstrument");
    // We get a 1:1 map by default so the detector ID should match the spectrum
    // number
    for (size_t i = 0; i < ws2->getNumberHistograms(); ++i) {
      // Create a detector for each spectra
      Detector *det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
      det->setShape(ComponentCreationHelper::createSphere(0.01, V3D(0, 0, 0), "1"));
      inst->add(det);
      inst->markAsDetector(det);
      ws2->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
    }
    ws2->setInstrument(inst);
    return ws2;
  }

  std::string outputFile;
  std::string entryName;
  std::string dataName;
  std::string title;
  Workspace2D myworkspace;

  Mantid::DataHandling::LoadRaw3 loader;
  std::string inputFile;
  std::string outputSpace;
  bool clearfiles;
};
