#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_

#include <cxxtest/TestSuite.h>
#include <Poco/Path.h>
#include <Poco/File.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"

using MantidQt::CustomInterfaces::MuonAnalysisDataLoader;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;
using MantidQt::CustomInterfaces::Muon::LoadResult;

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
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_ */