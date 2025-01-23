// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class LoadTest : public CxxTest::TestSuite {

private:
  std::vector<std::string> m_dataSearchDirs;
  std::string m_instName;

public:
  void setUp() override {

    m_dataSearchDirs = ConfigService::Instance().getDataSearchDirs();

    m_instName = ConfigService::Instance().getString("default.instrument");

    ConfigService::Instance().setString("default.facility", "ISIS");
  }

  void tearDown() override {

    ConfigService::Instance().setDataSearchDirs(m_dataSearchDirs);

    ConfigService::Instance().setString("default.facility", " ");

    ConfigService::Instance().setString("default.instrument", m_instName);

    AnalysisDataService::Instance().clear();
  }

  void testPropertyValues() {
    auto loader = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(loader->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(loader->existsProperty("OutputWorkspace"), true);

    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("Filename", "MUSR00022725.nxs"));
    TS_ASSERT_EQUALS(loader->existsProperty("EntryNumber"), true);
    TS_ASSERT_EQUALS(loader->existsProperty("AutoGroup"), true);
    TS_ASSERT_EQUALS(loader->existsProperty("MainFieldDirection"), true);
    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("SpectrumMin", "2"));
    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("SpectrumMax", "5"));

    // Execute & Test that the properties have the expected values
    loader->setPropertyValue("OutputWorkspace", "dummy");
    loader->setPropertyValue("DeadTimeTable", "dummy");
    loader->setChild(true);
    loader->execute();

    TS_ASSERT_EQUALS(static_cast<int>(loader->getProperty("SpectrumMin")), 2);
    TS_ASSERT_EQUALS(static_cast<int>(loader->getProperty("SpectrumMax")), 5);
    TS_ASSERT_EQUALS(loader->getPropertyValue("MainFieldDirection"), "Transverse");
    TS_ASSERT_DELTA(static_cast<double>(loader->getProperty("TimeZero")), 0.55, 1e-7);
    TS_ASSERT_DELTA(static_cast<double>(loader->getProperty("FirstGoodData")), 0.656, 1e-7);
    TS_ASSERT(static_cast<Workspace_sptr>(loader->getProperty("DeadTimeTable")));
  }

  void testSwitchingLoader() {
    auto loader = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(loader->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(loader->existsProperty("OutputWorkspace"), true);
    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("Filename", "IRS38633.raw"));
    TS_ASSERT_EQUALS(loader->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(loader->existsProperty("LoadLogFiles"), true);

    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("SpectrumMin", "10"));
    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("SpectrumMax", "100"));

    // Test that the properties have the correct values
    TS_ASSERT_EQUALS(loader->getPropertyValue("SpectrumMin"), "10");
    TS_ASSERT_EQUALS(loader->getPropertyValue("SpectrumMax"), "100");

    // Change loader
    loader->setPropertyValue("Filename", "LOQ49886.nxs");
    TS_ASSERT_EQUALS(loader->existsProperty("EntryNumber"), true);
    TS_ASSERT_EQUALS(loader->existsProperty("Cache"), false);

    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("SpectrumMin", "11"));
    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("SpectrumMax", "101"));

    TS_ASSERT_EQUALS(loader->getPropertyValue("SpectrumMin"), "11");
    TS_ASSERT_EQUALS(loader->getPropertyValue("SpectrumMax"), "101");
  }

  void testFindLoader() {
    Load loader;
    loader.initialize();
    static const size_t NUMPROPS = 5;
    const char *loadraw_props[NUMPROPS] = {"SpectrumMin", "SpectrumMax", "SpectrumList", "Cache", "LoadLogFiles"};
    // Basic load has no additional loader properties
    for (auto &loadraw_prop : loadraw_props) {
      TS_ASSERT_EQUALS(loader.existsProperty(loadraw_prop), false);
    }
    // After setting the file property, the algorithm should have aquired the
    // appropriate properties
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "IRS38633.raw"));
    // Now
    for (auto &loadraw_prop : loadraw_props) {
      TS_ASSERT_EQUALS(loader.existsProperty(loadraw_prop), true);
    }

    // Did it find the right loader
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadRaw");
  }

  void test_Comma_Separated_List_Finds_Correct_Number_Of_Files() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189,15190,15191.nxs");

    std::vector<std::vector<std::string>> foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(3, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());
    TS_ASSERT_EQUALS(1, foundFiles[2].size());
  }

  void test_Plus_Operator_Finds_Correct_Number_Of_Files() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "IRS38633+38633.nxs");

    std::vector<std::vector<std::string>> foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    TS_ASSERT_EQUALS(1, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(2, foundFiles[0].size());
  }

  void test_Range_Operator_Finds_Correct_Number_Of_Files() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189:15192.nxs");

    std::vector<std::vector<std::string>> foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(4, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());
    TS_ASSERT_EQUALS(1, foundFiles[2].size());
    TS_ASSERT_EQUALS(1, foundFiles[3].size());
  }

  void test_Stepped_Range_Operator_Finds_Correct_Number_Of_Files() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189:15192:2.nxs");

    std::vector<std::vector<std::string>> foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(2, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());

    // Check it has found the correct two
    const std::string first = foundFiles[0][0];
    TSM_ASSERT(std::string("Incorrect first file has been found: ") + first, first.ends_with("MUSR00015189.nxs"));
    const std::string second = foundFiles[1][0];
    TSM_ASSERT(std::string("Incorrect second file has been found") + second, second.ends_with("MUSR00015191.nxs"));

    // A more through test of the loading and value checking is done in the
    // LoadTest.py system test
  }

  void test_Added_Range_Operator_Finds_Correct_Number_Of_Files() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189-15192.nxs");

    std::vector<std::vector<std::string>> foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(1, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(4, foundFiles[0].size());

    // Check it has found the correct two
    const std::string first = foundFiles[0][0];
    TSM_ASSERT(std::string("Incorrect first file has been found: ") + first, first.ends_with("MUSR00015189.nxs"));
    const std::string last = foundFiles[0][3];
    TSM_ASSERT(std::string("Incorrect last file has been found") + last, last.ends_with("MUSR00015192.nxs"));

    // A more through test of the loading and value checking is done in the
    // LoadTest.py system test
  }

  void test_Comma_Separated_List_Of_Different_Intruments_Finds_Correct_Files() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48127.raw, CSP79590.raw");

    std::vector<std::vector<std::string>> foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(2, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());

    // Check it has found the correct two
    const std::string first = foundFiles[0][0];
    TSM_ASSERT(std::string("Incorrect first file has been found: ") + first, first.ends_with("LOQ48127.raw"));
    const std::string second = foundFiles[1][0];
    TSM_ASSERT(std::string("Incorrect second file has been found") + second, second.ends_with("CSP79590.raw"));
  }

  /*
   * This test loads and sums 2 IN4 runs from ILL
   * without instrument prefix in the file names.
   */
  void test_ILLINXLoadMultipleFilesNoPrefix() {

    ConfigService::Instance().setString("default.instrument", "IN4");
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN4/");

    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "084446+084447.nxs");

    std::string outputWS = AnalysisDataService::Instance().uniqueName(5, "LoadTest_");
    loader.setPropertyValue("OutputWorkspace", outputWS);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS);
    MatrixWorkspace_sptr output2D = std::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 397);
  }

  /*
   * This test loads and sums 2 IN4 runs from ILL
   * without instrument prefix and extension in the file names.
   */
  void test_ILLLoadMultipleFilesNoPrefixNoExt() {

    ConfigService::Instance().setString("default.instrument", "IN4");
    ConfigService::Instance().appendDataSearchSubDir("ILL/IN4/");

    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "084446-084447");

    std::string outputWS = AnalysisDataService::Instance().uniqueName(5, "LoadTest_");
    loader.setPropertyValue("OutputWorkspace", outputWS);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS);
    MatrixWorkspace_sptr output2D = std::dynamic_pointer_cast<MatrixWorkspace>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 397);
  }

  void test_EventPreNeXus_WithNoExecute() {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860_neutron_event.dat"));
    TS_ASSERT_EQUALS(loader.existsProperty("EventFilename"), false);
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadEventPreNexus");
  }

  void test_SNSEventNeXus_WithNoExecute() {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860_event.nxs"));
    TS_ASSERT_EQUALS(loader.existsProperty("EventFilename"), false);
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadEventNexus");
  }

  void testArgusFileWithIncorrectZeroPadding_NoExecute() {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "argus0026287.nxs");
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadMuonNexus");
  }

  void test_must_set_loadername() {
    std::string const outputWS = AnalysisDataService::Instance().uniqueName(5, "LoadTest_");
    std::string const incorrectLoader = "NotALoader";
    int const incorrectVersion = -2;

    Load loader;
    // run Load with the LoaderName set to something
    // verify that at the end, it is correctly set back according to the output
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860_event.nxs"));
    // the loader namer will be set: grab it an ensure it is not the bad cvalue
    std::string const correctLoader = loader.getPropertyValue("LoaderName");
    int const correctVersion = loader.getProperty("LoaderVersion");
    TS_ASSERT_DIFFERS(correctLoader, incorrectLoader);
    TS_ASSERT_DIFFERS(correctVersion, incorrectVersion);
    // now SET the loader to a bad value, and execute
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("LoaderName", incorrectLoader));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("LoaderVersion", incorrectVersion));
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), incorrectLoader);
    TS_ASSERT_EQUALS((int)loader.getProperty("LoaderVersion"), incorrectVersion);
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    // make sure the loader name has been correctly set
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), correctLoader);
    TS_ASSERT_EQUALS((int)loader.getProperty("LoaderVersion"), correctVersion);
  }
};

//-------------------------------------------------------------------------------------------------
// Performance test
//
// This simple checks how long it takes to run the search for a Loader, which is
// done when
// the file property is set
//-------------------------------------------------------------------------------------------------

class LoadTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadTestPerformance *createSuite() { return new LoadTestPerformance(); }
  static void destroySuite(LoadTestPerformance *suite) { delete suite; }

  void test_find_loader_performance() {
    const size_t ntimes(5);

    for (size_t i = 0; i < ntimes; ++i) {
      Mantid::DataHandling::Load loader;
      loader.initialize();
      loader.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    }
  }
};
