// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed2.h"
#include "MantidDataHandling/SaveNexusESS.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidFrameworkTestHelpers/NexusFileReader.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include <memory>

using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {

template <typename T> void do_execute(const std::string &filename, T &ws, bool append = false) {
  SaveNexusESS alg;
  alg.setChild(true);
  alg.setRethrows(true);
  alg.initialize();
  alg.isInitialized();
  alg.setProperty("InputWorkspace", ws);
  alg.setProperty("Filename", filename);
  alg.setProperty("Append", append);
  alg.execute();
  alg.isExecuted();
}

namespace test_utility {

Mantid::API::MatrixWorkspace_sptr reload(const std::string &filename) {
  LoadNexusProcessed2 loader;
  loader.setChild(true);
  loader.setRethrows(true);
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  Workspace_sptr out = loader.getProperty("OutputWorkspace");
  auto matrixWSOut = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(out);
  return matrixWSOut;
}

Mantid::API::MatrixWorkspace_sptr from_instrument_file(const std::string &filename) {
  LoadEmptyInstrument loader;
  loader.setChild(true);
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");
  return ws;
}

Mantid::API::MatrixWorkspace_sptr from_instrument_name(const std::string &name) {
  LoadEmptyInstrument loader;
  loader.setChild(true);
  loader.initialize();
  loader.setProperty("InstrumentName", name);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");
  return ws;
}

Mantid::API::MatrixWorkspace_sptr load(const std::string &name) { return reload(name); }

} // namespace test_utility
} // namespace

class SaveNexusESSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusESSTest *createSuite() { return new SaveNexusESSTest(); }
  static void destroySuite(SaveNexusESSTest *suite) { delete suite; }

  void test_Init() {
    SaveNexusESS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_rectangular_instrument_details() {
    using namespace Mantid::NexusGeometry;

    FileResource fileInfo("test_rectangular_instrument.nxs");

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1 /*numBanks*/, 10 /*numPixels*/,
                                                                                  10 /*numBins*/);

    const auto &inDetInfo = ws->detectorInfo();
    const auto &inCompInfo = ws->componentInfo();

    do_execute(fileInfo.fullPath(), ws);

    // Load and check instrument geometry
    Mantid::Kernel::Logger logger("test_logger");
    auto instr = NexusGeometryParser::createInstrument(fileInfo.fullPath(), makeLogger(&logger));

    Mantid::Geometry::ParameterMap pmap;
    auto beamline = instr->makeBeamline(pmap);
    const auto &outDetInfo = beamline.second;
    const auto &outCompInfo = beamline.first;
    // Same detector info
    TS_ASSERT(outDetInfo->isEquivalent(inDetInfo));
    // We have a 10 by 10 Rectangular Detector Bank, this means 10 columns.
    // SaveNexusGeometry (via SaveNexusESS) will not save columns of a
    // Rectangular detector bank. Hence subtranction from output.
    TS_ASSERT_EQUALS(outCompInfo->size(), inCompInfo.size() - 10);
  }

  void test_exec_rectangular_data() {
    FileResource fileInfo("test_rectangular_data.nxs");
    auto wsIn = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1 /*numBanks*/, 10 /*numPixels*/,
                                                                                    12 /*numBins*/);

    do_execute(fileInfo.fullPath(), wsIn);

    auto matrixWSOut = test_utility::reload(fileInfo.fullPath());

    TS_ASSERT_EQUALS(matrixWSOut->blocksize(), 12);
    TS_ASSERT_EQUALS(matrixWSOut->getNumberHistograms(), 10 * 10);
    TS_ASSERT(matrixWSOut->detectorInfo().isEquivalent(wsIn->detectorInfo()));
  }

  void test_with_ess_instrument() {
    using namespace Mantid::HistogramData;

    FileResource fileInfo("test_ess_instrument.nxs");
    auto wsIn = test_utility::from_instrument_file("V20_4-tubes_90deg_Definition_v01.xml");
    for (size_t i = 0; i < wsIn->getNumberHistograms(); ++i) {
      wsIn->setCounts(i, Counts{double(i)});
    }

    do_execute(fileInfo.fullPath(), wsIn);
    auto wsOut = test_utility::reload(fileInfo.fullPath());

    // Quick geometry Test
    TS_ASSERT(wsOut->detectorInfo().isEquivalent(wsIn->detectorInfo()));

    // Quick data test.
    for (size_t i = 0; i < wsIn->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(wsIn->counts(i)[0], wsOut->counts(i)[0]);
    }
  }

  void test_demonstrate_spectra_detector_map_saved() {
    FileResource fileInfo("test_spectra_mapping.nxs");
    auto wsIn = createWorkspaceWithInstrumentAndSpectraMap("basic_rect");

    do_execute(fileInfo.fullPath(), wsIn);

    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_1",
                                            wsIn->componentInfo().name(wsIn->componentInfo().root()));
  }

  void test_append_multiple_workspaces() {
    FileResource fileInfo("test_multiple_workspaces.nxs");
    auto ws1 = createWorkspaceWithInstrumentAndSpectraMap("first_instrument");
    auto ws2 = createWorkspaceWithInstrumentAndSpectraMap("second_instrument");
    auto ws3 = createWorkspaceWithInstrumentAndSpectraMap("third_instrument");

    // write NXentry: "mantid_workspace_1"
    do_execute(fileInfo.fullPath(), ws1);
    // write NXentry: "mantid_workspace_2"
    do_execute(fileInfo.fullPath(), ws2, true);
    // write NXentry: "mantid_workspace_3"
    do_execute(fileInfo.fullPath(), ws3, true);

    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_1", "first_instrument");

    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_2", "second_instrument");

    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_3", "third_instrument");
  }

  void test_workspace_group() {
    FileResource fileInfo("test_group_workspace.nxs");
    auto ws1 = createWorkspaceWithInstrumentAndSpectraMap("first_instrument");
    auto ws2 = createWorkspaceWithInstrumentAndSpectraMap("second_instrument");
    auto ws3 = createWorkspaceWithInstrumentAndSpectraMap("third_instrument");

    auto &ADS = AnalysisDataService::Instance();
    ADS.add("ws1", ws1);
    ADS.add("ws2", ws2);
    ADS.add("ws3", ws3);
    auto wss = groupWorkspaces("wss", {"ws1", "ws2", "ws3"});

    // Write three NXentry, from the unrolled group workspace:
    //   "mantid_workspace_1", "mantid_workspace_2", and "mantid_workspace_3".
    do_execute(fileInfo.fullPath(), wss);

    // File structure should be exactly the same as if the unrolled
    //   workspaces were appended separately.
    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_1", "first_instrument");

    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_2", "second_instrument");

    validate_spectra_detector_map_structure(fileInfo.fullPath(), "mantid_workspace_3", "third_instrument");

    ADS.remove("wss");
    ADS.remove("ws1");
    ADS.remove("ws2");
    ADS.remove("ws3");
  }

  void test_saveInstrument_with_workspace() {

    // This is testing the core routine, but we put it here and not in
    // NexusGeometrySave because we need access to WorkspaceCreationHelpers for
    // this.

    FileResource fileResource("test_with_full_workspace.hdf5");
    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);

    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20);

    Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(*ws, fileResource.fullPath(), "entry", adapter);
  }

  void test_saveInstrument_with_multiple_workspace_entries() {

    // Test that the correct instruments are appended to the NXentries, when entry numbers are specified:
    //   for this specific test, none of the NXentry parent groups will exist in advance.

    // As with the previous test, this test is placed here because it requires `WorkspaceCreationHelper`.

    using Mantid::NexusGeometry::NX_ENTRY;
    using Mantid::NexusGeometry::NX_INSTRUMENT;

    const size_t N_workspace_entries = 3;
    FileResource testInput("test_with_multiple_workspace_entries.hdf5");
    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);

    for (size_t n = 1; n <= N_workspace_entries; ++n) {
      std::ostringstream name;
      name << "instrument_" << n;
      auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20, name.str());
      Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(*ws, testInput.fullPath(), "mantid_workspace_", n,
                                                               adapter, n > 1);
    }

    H5::H5File h5(testInput.fullPath(), H5F_ACC_RDONLY);
    _assert_group_structure(h5, {
                                    {"/mantid_workspace_1", NX_ENTRY},
                                    {"/mantid_workspace_1/instrument_1", NX_INSTRUMENT},
                                    {"/mantid_workspace_2", NX_ENTRY},
                                    {"/mantid_workspace_2/instrument_2", NX_INSTRUMENT},
                                    {"/mantid_workspace_3", NX_ENTRY},
                                    {"/mantid_workspace_3/instrument_3", NX_INSTRUMENT},
                                });
  }

  void test_saveInstrument_with_multiple_existing_workspace_entries() {

    // Test that the instruments are appended to the correct NXentries, when entry numbers are specified:
    //   for this specific test, each of the NXentry parent groups will exist in advance.

    // As with the previous test, this test is placed here because it requires `WorkspaceCreationHelper`.

    using Mantid::NexusGeometry::NX_CLASS;
    using Mantid::NexusGeometry::NX_ENTRY;
    using Mantid::NexusGeometry::NX_INSTRUMENT;

    FileResource testInput("test_with_multiple_existing_workspace_entries.hdf5");

    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);

    const size_t N_workspace_entries = 3;
    {
      // Create several NXentry.
      H5::H5File h5(testInput.fullPath(), H5F_ACC_TRUNC);
      for (size_t n = 1; n < N_workspace_entries; ++n) {
        std::ostringstream entryName;
        entryName << "/mantid_workspace_" << n;
        std::cout << "creating: " << entryName.str() << std::endl;
        H5::Group g = h5.createGroup(entryName.str());
        H5Util::writeStrAttribute(g, NX_CLASS, NX_ENTRY);
      }
      h5.close();
    }

    // Write an instrument to each NXentry.
    for (size_t n = 1; n <= N_workspace_entries; ++n) {
      std::ostringstream name;
      name << "instrument_" << n;
      auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20, name.str());
      Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(*ws, testInput.fullPath(), "mantid_workspace_", n,
                                                               adapter, true);
    }

    H5::H5File h5(testInput.fullPath(), H5F_ACC_RDONLY);
    _assert_group_structure(h5, {
                                    {"/mantid_workspace_1", NX_ENTRY},
                                    {"/mantid_workspace_1/instrument_1", NX_INSTRUMENT},
                                    {"/mantid_workspace_2", NX_ENTRY},
                                    {"/mantid_workspace_2/instrument_2", NX_INSTRUMENT},
                                    {"/mantid_workspace_3", NX_ENTRY},
                                    {"/mantid_workspace_3/instrument_3", NX_INSTRUMENT},
                                });
  }

  void test_saveInstrument_with_multiple_workspaces_append() {

    // Test that the instrument is appended to the correct NXentry,
    //   when an explicit entry number is not specified.

    // As with the previous test, this test is placed here because it requires `WorkspaceCreationHelper`.

    using Mantid::NexusGeometry::NX_CLASS;
    using Mantid::NexusGeometry::NX_ENTRY;
    using Mantid::NexusGeometry::NX_INSTRUMENT;

    FileResource testInput("test_with_multiple_workspaces_append.hdf5");
    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);

    // Create an HDF5 file with several NXentry.
    const size_t N_workspace_entries = 3;
    for (size_t n = 1; n <= N_workspace_entries; ++n) {
      {
        // Write the latest NXentry.
        H5::H5File h5(testInput.fullPath(), n > 1 ? H5F_ACC_RDWR : H5F_ACC_TRUNC);
        std::ostringstream entryName;
        entryName << "/mantid_workspace_" << n;
        H5::Group g = h5.createGroup(entryName.str());
        H5Util::writeStrAttribute(g, NX_CLASS, NX_ENTRY);
        h5.close();
      }
      {
        // Write the corresponding NXinstrument.
        std::ostringstream instrumentName;
        instrumentName << "instrument_" << n;
        auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20, instrumentName.str());
        Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(*ws, testInput.fullPath(), "mantid_workspace_",
                                                                 std::nullopt, adapter, true);
      }
    }

    // Verify the resulting structure.
    H5::H5File h5(testInput.fullPath(), H5F_ACC_RDONLY);
    _assert_group_structure(h5, {
                                    {"/mantid_workspace_1", NX_ENTRY},
                                    {"/mantid_workspace_1/instrument_1", NX_INSTRUMENT},
                                    {"/mantid_workspace_2", NX_ENTRY},
                                    {"/mantid_workspace_2/instrument_2", NX_INSTRUMENT},
                                    {"/mantid_workspace_3", NX_ENTRY},
                                    {"/mantid_workspace_3/instrument_3", NX_INSTRUMENT},
                                });
  }

  void test_regression_iris() {
    FileResource handle("test_regression_iris.nxs"); // IRIS has single monitors

    auto iris = test_utility::from_instrument_name("IRIS");
    do_execute(handle.fullPath(), iris);
    auto iris_reloaded = test_utility::reload(handle.fullPath());
    const auto &indexInfo = iris->indexInfo();
    const auto &indexInfoReload = iris_reloaded->indexInfo();
    const auto &outDetInfo = iris_reloaded->detectorInfo();
    const auto &inDetInfo = iris->detectorInfo();
    TS_ASSERT_EQUALS(inDetInfo.size(), outDetInfo.size());
    TS_ASSERT_EQUALS(indexInfo.size(), indexInfoReload.size());
  }

  void test_not_all_detectors_mapped_to_spectrum() {
    FileResource handle("test_regression_iris_with_mappings.nxs"); // IRIS does not include all
                                                                   // detectors in it's
                                                                   // mappings.
    auto ws = test_utility::load("irs26176_graphite002_red.nxs");
    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(*ws, handle.fullPath(), "entry", adapter));
  }

  void test_not_all_detectors_mapped_to_spectrum_and_reloaded() {
    FileResource handle("test_regression_iris_with_mappings.nxs"); // IRIS does not include all
                                                                   // detectors in it's
                                                                   // mappings.
    auto ws = test_utility::load("irs26176_graphite002_red.nxs");
    do_execute(handle.fullPath(), ws);
    auto ws_out = test_utility::reload(handle.fullPath());
  }

private:
  // Validate the spectra and detector-map structure of a single workspace entry.
  static void validate_spectra_detector_map_structure(const std::string &filePath, const std::string &parentEntryName,
                                                      const std::string &instrumentName) {
    Mantid::NexusGeometry::NexusFileReader validator(filePath);
    TS_ASSERT(validator.hasDataset("spectra", {parentEntryName, instrumentName, "bank1"}));
    TS_ASSERT(validator.hasDataset("detector_list", {parentEntryName, instrumentName, "bank1"}));
    TS_ASSERT(validator.hasDataset("detector_index", {parentEntryName, instrumentName, "bank1"}));
    TS_ASSERT(validator.hasDataset("detector_count", {parentEntryName, instrumentName, "bank1"}));
    TS_ASSERT(validator.hasDataset("spectra", {parentEntryName, instrumentName, "bank2"}));
    TS_ASSERT(validator.hasDataset("detector_list", {parentEntryName, instrumentName, "bank2"}));
    TS_ASSERT(validator.hasDataset("detector_index", {parentEntryName, instrumentName, "bank2"}));
    TS_ASSERT(validator.hasDataset("detector_count", {parentEntryName, instrumentName, "bank2"}));
  }

  // Create a 2D workspace with spectra and a simple, named instrument.
  static Mantid::DataObjects::Workspace2D_sptr
  createWorkspaceWithInstrumentAndSpectraMap(const std::string &instrumentName) {
    using namespace Mantid::Indexing;
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2 /*numBanks*/, 10 /*numPixels*/,
                                                                                  12 /*numBins*/, instrumentName);
    std::vector<SpectrumDefinition> specDefinitions;
    std::vector<SpectrumNumber> spectrumNumbers;
    size_t i = ws->getNumberHistograms() - 1;
    for (size_t j = 0; j < ws->getNumberHistograms(); --i, ++j) {
      specDefinitions.emplace_back(SpectrumDefinition(i));
      spectrumNumbers.emplace_back(SpectrumNumber(static_cast<int>(j)));
    }
    IndexInfo info(spectrumNumbers);
    info.setSpectrumDefinitions(specDefinitions);
    ws->setIndexInfo(info);
    return ws;
  }

  // Create a group workspace from several input workspaces.
  static Mantid::API::Workspace_sptr groupWorkspaces(const std::string &outputWSName,
                                                     const std::vector<std::string> &inputWSNames) {
    const auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("OutputWorkspace", outputWSName);
    groupAlg->setProperty("InputWorkspaces", inputWSNames);
    groupAlg->execute();
    return AnalysisDataService::Instance().retrieve(outputWSName);
  }

  // Verify that multiple HDF5 groups, with a specified NX_class, exist in an HDF5 file at the expected locations
  void _assert_group_structure(H5::H5File &file,
                               const std::vector<std::pair<std::string, std::string>> &pathsWithClasses) {
    using Mantid::NexusGeometry::NX_CLASS;

    for (const auto &pathWithClass : pathsWithClasses) {
      const std::string &groupPath = pathWithClass.first;
      const std::string &className = pathWithClass.second;
      TS_ASSERT(H5Util::groupExists(file, groupPath));
      H5::Group g = file.openGroup(groupPath);
      TS_ASSERT(H5Util::keyHasValue(g, NX_CLASS, className));
    }
  }
};
