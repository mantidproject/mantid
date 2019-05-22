// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_

#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

#include "../Muon/MuonAnalysisDataLoader.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"

using Mantid::API::IAlgorithm_sptr;
using Mantid::API::TableRow;
using Mantid::API::Workspace;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;
using MantidQt::CustomInterfaces::Muon::AnalysisOptions;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;
using MantidQt::CustomInterfaces::Muon::ItemType;
using MantidQt::CustomInterfaces::Muon::LoadResult;
using MantidQt::CustomInterfaces::Muon::PlotType;
using MantidQt::CustomInterfaces::MuonAnalysisDataLoader;

/// Inherits from class under test so that protected methods can be tested
class TestDataLoader : public MuonAnalysisDataLoader {
public:
  TestDataLoader(const DeadTimesType &deadTimesType,
                 const QStringList &instruments,
                 const std::string &deadTimesFile = "")
      : MuonAnalysisDataLoader(deadTimesType, instruments, deadTimesFile){};
  void setProcessAlgorithmProperties(IAlgorithm_sptr alg,
                                     const AnalysisOptions &options) const {
    MuonAnalysisDataLoader::setProcessAlgorithmProperties(alg, options);
  }
};

class MuonAnalysisDataLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisDataLoaderTest *createSuite() {
    return new MuonAnalysisDataLoaderTest();
  }
  static void destroySuite(MuonAnalysisDataLoaderTest *suite) { delete suite; }

  /// Constructor
  MuonAnalysisDataLoaderTest() {
    // To make sure API is initialized properly
    Mantid::API::FrameworkManager::Instance();
  }

  /// Test loading a file whose instrument is not in the list
  void test_loadFiles_badInstrument() {
    QStringList instruments, files;
    instruments << "MUSR"
                << "HIFI";
    files << "emu00006473.nxs";
    MuonAnalysisDataLoader loader(DeadTimesType::None, instruments);
    TS_ASSERT_THROWS(loader.loadFiles(files), const std::runtime_error &);
  }

  /// Test special case for DEVA files
  void test_loadFiles_DEVA() {
    QStringList instruments, files;
    instruments << "MUSR"
                << "HIFI";
    files << "DEVA01360.nxs";
    MuonAnalysisDataLoader loader(DeadTimesType::None, instruments);
    LoadResult result;
    TS_ASSERT_THROWS_NOTHING(result = loader.loadFiles(files));
    // Test that it's loaded
    TS_ASSERT_EQUALS(result.label, "DEVA000");
    TS_ASSERT_EQUALS(result.mainFieldDirection, "Longitudinal");
    const auto loadedWS = result.loadedWorkspace;
    TS_ASSERT(loadedWS);
    // Test that there are 2 periods
    const auto wsGroup =
        boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(loadedWS);
    TS_ASSERT(wsGroup);
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
    // Test that there are 6 spectra per period
    for (int i = 0; i < 2; i++) {
      const auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          wsGroup->getItem(i));
      TS_ASSERT(ws);
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 6);
      TS_ASSERT_EQUALS(ws->getInstrument()->getName(), "DEVA");
    }
  }

  void test_loadFiles_multiple() {
    QStringList instruments, files;
    instruments << "MUSR"
                << "HIFI";
    files << "MUSR00015189.nxs"
          << "MUSR00015190.nxs";
    MuonAnalysisDataLoader loader(DeadTimesType::None, instruments);
    LoadResult result;
    TS_ASSERT_THROWS_NOTHING(result = loader.loadFiles(files));
    TS_ASSERT_EQUALS(result.label, "MUSR00015189-90");
    const auto loadedWS = result.loadedWorkspace;
    TS_ASSERT(loadedWS);
    // Test that there are 2 periods
    const auto wsGroup =
        boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(loadedWS);
    TS_ASSERT(wsGroup);
    TS_ASSERT_EQUALS(wsGroup->getNumberOfEntries(), 2);
  }

  void test_getDeadTimesTable_None() {
    MuonAnalysisDataLoader loader(DeadTimesType::None, {"MUSR"});
    LoadResult result;
    const auto deadTimes = loader.getDeadTimesTable(result);
    TS_ASSERT(!deadTimes);
  }

  void test_getDeadTimesTable_FromFile_NotPresent() {
    MuonAnalysisDataLoader loader(DeadTimesType::FromFile, {"MUSR"});
    LoadResult result;
    TS_ASSERT_THROWS(loader.getDeadTimesTable(result), const std::runtime_error &);
  }

  void test_getDeadTimesTable_FromFile() {
    MuonAnalysisDataLoader loader(DeadTimesType::FromFile, {"MUSR"});
    LoadResult result;
    const auto deadTimes = createDeadTimeTable({1, 2, 3}, {0.1, 0.2, 0.3});
    result.loadedDeadTimes = deadTimes;
    const auto loadedDeadTimes = loader.getDeadTimesTable(result);
    TS_ASSERT_EQUALS(deadTimes, loadedDeadTimes);
  }

  void test_getDeadTimesTable_FromFile_WorkspaceGroup() {
    MuonAnalysisDataLoader loader(DeadTimesType::FromFile, {"MUSR"});
    LoadResult result;
    const auto deadTimes = createDeadTimeTable({1, 2, 3}, {0.1, 0.2, 0.3});
    auto wsGroup = boost::make_shared<WorkspaceGroup>();
    wsGroup->addWorkspace(deadTimes);
    result.loadedDeadTimes = boost::dynamic_pointer_cast<Workspace>(wsGroup);
    const auto loadedDeadTimes = loader.getDeadTimesTable(result);
    TS_ASSERT_EQUALS(deadTimes, loadedDeadTimes);
  }

  void test_getDeadTimesTable_FromDisk() {
    const auto deadTimes = createDeadTimeTable({1, 2, 3}, {0.1, 0.2, 0.3});
    auto save = Mantid::API::AlgorithmFactory::Instance().create(
        "SaveNexusProcessed", 1);
    save->initialize();
    save->setChild(true);
    save->setProperty("InputWorkspace",
                      boost::dynamic_pointer_cast<Workspace>(deadTimes));
    Poco::Path tempFile(Poco::Path::temp());
    tempFile.setFileName("tempdeadtimes.nxs");
    save->setPropertyValue("Filename", tempFile.toString());
    save->execute();
    MuonAnalysisDataLoader loader(DeadTimesType::FromDisk, {"MUSR"},
                                  tempFile.toString());
    const auto loadedDeadTimes = loader.getDeadTimesTable(LoadResult());
    for (size_t i = 0; i < 3; i++) {
      TS_ASSERT_EQUALS(loadedDeadTimes->cell<int>(i, 0),
                       deadTimes->cell<int>(i, 0));
      TS_ASSERT_EQUALS(loadedDeadTimes->cell<double>(i, 1),
                       deadTimes->cell<double>(i, 1));
    }
    Poco::File(tempFile).remove();
  }

  void test_correctAndGroup() {
    MuonAnalysisDataLoader loader(DeadTimesType::FromFile, {"MUSR"});
    LoadResult result;
    TS_ASSERT_THROWS_NOTHING(result = loader.loadFiles({"MUSR00015189.nxs"}));
    Mantid::API::Grouping grouping;
    grouping.groupNames = {"fwd", "bwd"};
    grouping.groups = {"33-64", "1-32"};
    grouping.pairNames = {"long"};
    grouping.pairs.emplace_back(1, 0);
    Mantid::API::Workspace_sptr corrected;
    TS_ASSERT_THROWS_NOTHING(corrected =
                                 loader.correctAndGroup(result, grouping));
    auto correctedGroup =
        boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(corrected);
    TS_ASSERT(correctedGroup);
    TS_ASSERT_EQUALS(correctedGroup->size(), 2);
    for (size_t i = 0; i < correctedGroup->size(); i++) {
      auto matrixWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          correctedGroup->getItem(i));
      TS_ASSERT(matrixWS);
      // Check that each period has number of spectra = number of groups
      TS_ASSERT_EQUALS(matrixWS->getNumberHistograms(), grouping.groups.size());
      // Check that each period has been corrected for dead time
      TS_ASSERT_DELTA(matrixWS->getSpectrum(0).y()[0],
                      i == 0 ? 84.1692 : 16.0749, 0.0001);
    }
  }

  void test_setProcessAlgorithmProperties_GroupCounts() {
    doTest_setAlgorithmProperties(ItemType::Group, PlotType::Counts, "0.08");
  }

  void test_setProcessAlgorithmProperties_GroupCounts_NoRebin() {
    doTest_setAlgorithmProperties(ItemType::Group, PlotType::Counts, "");
  }

  void test_setProcessAlgorithmProperties_GroupLog() {
    doTest_setAlgorithmProperties(ItemType::Group, PlotType::Logarithm, "");
  }

  void test_setProcessAlgorithmProperties_GroupAsym() {
    doTest_setAlgorithmProperties(ItemType::Group, PlotType::Asymmetry, "");
  }

  void test_setProcessAlgorithmProperties_PairAsym() {
    doTest_setAlgorithmProperties(ItemType::Pair, PlotType::Asymmetry, "");
  }

  void test_setProcessAlgorithmProperties_PairCounts_Throws() {
    doTest_setAlgorithmProperties(ItemType::Pair, PlotType::Counts, "", true);
  }

  void test_setProcessAlgorithmProperties_PairLog_Throws() {
    doTest_setAlgorithmProperties(ItemType::Pair, PlotType::Logarithm, "",
                                  true);
  }

  void test_createAnalysisWorkspace() {
    MuonAnalysisDataLoader loader(DeadTimesType::FromFile, {"MUSR"});
    LoadResult result;
    TS_ASSERT_THROWS_NOTHING(result = loader.loadFiles({"MUSR00015189.nxs"}));
    Mantid::API::Grouping grouping;
    grouping.groupNames = {"fwd", "bwd"};
    grouping.groups = {"33-64", "1-32"};
    grouping.pairNames = {"long"};
    grouping.pairs.emplace_back(0, 1);
    grouping.pairAlphas = {1.0};
    Mantid::API::Workspace_sptr corrected;
    TS_ASSERT_THROWS_NOTHING(corrected =
                                 loader.correctAndGroup(result, grouping));
    Mantid::API::Workspace_sptr analysed;
    AnalysisOptions options(grouping);
    options.groupPairName = "long";
    options.loadedTimeZero = 0.55;
    options.plotType = PlotType::Asymmetry;
    options.rebinArgs = "";
    options.subtractedPeriods = "2";
    options.summedPeriods = "1";
    options.timeLimits.first = 0.11;
    options.timeLimits.second = 10.0;
    options.timeZero = 0.55;
    TS_ASSERT_THROWS_NOTHING(
        analysed = loader.createAnalysisWorkspace(corrected, options));
    // test the output
    Mantid::MantidVec expectedOutput = {
        -0.037308, -0.0183329, 0.0250825, -0.0154756, 0.018308,
        0.0116216, -0.019053,  0.0100087, -0.0393029, -0.001696};
    const auto outputWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(analysed);
    TS_ASSERT(outputWS);
    const auto &data = outputWS->y(0);
    TS_ASSERT_EQUALS(data.size(), 1958);
    const auto &xData = outputWS->x(0);
    auto offset = std::distance(xData.begin(),
                                std::lower_bound(xData.begin(), xData.end(),
                                                 options.timeLimits.first)) +
                  1;
    for (size_t i = 0; i < expectedOutput.size(); i++) {
      TS_ASSERT_DELTA(data[i + offset], expectedOutput[i], 1e-6);
    }
  }

private:
  /**
   * Creates Dead Time Table using all the data between begin and end.
   * @param specToLoad :: vector containing the spectrum numbers to load
   * @param deadTimes :: vector containing the corresponding dead times
   * @return Dead Time Table created using the data
   */
  TableWorkspace_sptr createDeadTimeTable(std::vector<int> specToLoad,
                                          std::vector<double> deadTimes) {
    TableWorkspace_sptr deadTimeTable =
        boost::dynamic_pointer_cast<TableWorkspace>(
            WorkspaceFactory::Instance().createTable("TableWorkspace"));

    deadTimeTable->addColumn("int", "spectrum");
    deadTimeTable->addColumn("double", "dead-time");

    for (size_t i = 0; i < specToLoad.size(); i++) {
      TableRow row = deadTimeTable->appendRow();
      row << specToLoad[i] << deadTimes[i];
    }

    return deadTimeTable;
  }

  /**
   * Test the method "setAlgorithmProperties" with provided options
   * @param item :: [input] Item type group/pair
   * @param plot :: [input] Plot type counts/log/asym
   * @param rebinArgs :: [input] Arguments for rebin
   * @param shouldThrow :: [input, optional] If call should fail or not (default
   * false)
   */
  void doTest_setAlgorithmProperties(ItemType item, PlotType plot,
                                     const std::string &rebinArgs,
                                     bool shouldThrow = false) {
    TestDataLoader loader(DeadTimesType::FromFile, {"MUSR"});
    Mantid::API::Grouping grouping;
    grouping.groupNames = {"fwd", "bwd"};
    grouping.groups = {"33-64", "1-32"};
    grouping.pairNames = {"long"};
    grouping.pairs.emplace_back(1, 0);
    grouping.pairAlphas = {1.0};

    auto alg =
        Mantid::API::AlgorithmFactory::Instance().create("MuonProcess", 1);
    alg->initialize();

    AnalysisOptions options(grouping);
    // set options
    options.groupPairName = item == ItemType::Group ? "bwd" : "long";
    options.loadedTimeZero = 0.012;
    options.plotType = plot;
    options.rebinArgs = rebinArgs;
    options.subtractedPeriods = "2";
    options.summedPeriods = "1";
    options.timeLimits.first = 0.1;
    options.timeLimits.second = 10.0;
    options.timeZero = 0.014;

    if (shouldThrow) {
      TS_ASSERT_THROWS(loader.setProcessAlgorithmProperties(alg, options),
                       const std::invalid_argument &);
    } else {
      TS_ASSERT_THROWS_NOTHING(
          loader.setProcessAlgorithmProperties(alg, options));
      // test options == alg props
      TS_ASSERT_EQUALS(alg->getPropertyValue("Mode"), "Analyse");
      TS_ASSERT_EQUALS((double)alg->getProperty("TimeZero"), options.timeZero);
      TS_ASSERT_EQUALS((double)alg->getProperty("LoadedTimeZero"),
                       options.loadedTimeZero);
      TS_ASSERT_EQUALS((double)alg->getProperty("Xmin"),
                       options.timeLimits.first);
      TS_ASSERT_EQUALS((double)alg->getProperty("Xmax"),
                       options.timeLimits.second);
      TS_ASSERT_EQUALS(alg->getPropertyValue("RebinParams"), options.rebinArgs);
      const std::string outputType = alg->getPropertyValue("OutputType");
      if (item == ItemType::Group) {
        TS_ASSERT_EQUALS((int)alg->getProperty("GroupIndex"), 1);
        if (plot == PlotType::Asymmetry) {
          TS_ASSERT_EQUALS(outputType, "GroupAsymmetry");
        } else {
          TS_ASSERT_EQUALS(outputType, "GroupCounts");
        }
      } else {
        TS_ASSERT_EQUALS(outputType, "PairAsymmetry");
        TS_ASSERT_EQUALS((int)alg->getProperty("PairFirstIndex"), 1);
        TS_ASSERT_EQUALS((int)alg->getProperty("PairSecondIndex"), 0);
        TS_ASSERT_EQUALS((double)alg->getProperty("Alpha"), 1.0);
      }
    }
  }
};
#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_ */
