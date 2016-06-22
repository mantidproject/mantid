#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_

#include <cxxtest/TestSuite.h>
#include <Poco/Path.h>
#include <Poco/File.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"

using MantidQt::CustomInterfaces::MuonAnalysisDataLoader;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;
using MantidQt::CustomInterfaces::Muon::LoadResult;
using Mantid::API::TableRow;
using Mantid::API::Workspace;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;

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
    TS_ASSERT_THROWS(loader.loadFiles(files), std::runtime_error);
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
    TS_ASSERT_THROWS(loader.getDeadTimesTable(result), std::runtime_error);
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
    for (int i = 0; i < correctedGroup->size(); i++) {
      auto matrixWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          correctedGroup->getItem(i));
      TS_ASSERT(matrixWS);
      // Check that each period has number of spectra = number of groups
      TS_ASSERT_EQUALS(matrixWS->getNumberHistograms(), grouping.groups.size());
      // Check that each period has been corrected for dead time
      TS_ASSERT_DELTA(matrixWS->getSpectrum(0).dataY().at(0),
                      i == 0 ? 84.1692 : 16.0749, 0.0001);
    }
  }

private:
  /**
   * Creates Dead Time Table using all the data between begin and end.
   * @param specToLoad :: vector containing the spectrum numbers to load
   * @param deadTimes :: vector containing the corresponding dead times
   * @return Dead Time Table create using the data
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
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_ */