// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
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
template <typename T> void do_execute(const std::string &filename, T &ws) {
  SaveNexusESS alg;
  alg.setChild(true);
  alg.setRethrows(true);
  alg.initialize();
  alg.isInitialized();
  alg.setProperty("InputWorkspace", ws);
  alg.setProperty("Filename", filename);
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

    using namespace Mantid::Indexing;
    FileResource fileInfo("test_spectra_mapping.nxs");
    auto wsIn = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2 /*numBanks*/, 10 /*numPixels*/,
                                                                                    12 /*numBins*/);

    std::vector<SpectrumDefinition> specDefinitions;
    std::vector<SpectrumNumber> spectrumNumbers;
    size_t i = wsIn->getNumberHistograms() - 1;
    for (size_t j = 0; j < wsIn->getNumberHistograms(); --i, ++j) {
      specDefinitions.emplace_back(SpectrumDefinition(i));
      spectrumNumbers.emplace_back(SpectrumNumber(static_cast<int>(j)));
    }
    IndexInfo info(spectrumNumbers);
    info.setSpectrumDefinitions(specDefinitions);
    wsIn->setIndexInfo(info);
    do_execute(fileInfo.fullPath(), wsIn);

    {
      const auto rootName = wsIn->componentInfo().name(wsIn->componentInfo().root());
      // Check that mapping datasets are written
      Mantid::NexusGeometry::NexusFileReader validator(fileInfo.fullPath());
      TS_ASSERT(validator.hasDataset("spectra", {"mantid_workspace_1", rootName, "bank1"}));
      TS_ASSERT(validator.hasDataset("detector_list", {"mantid_workspace_1", rootName, "bank1"}));
      TS_ASSERT(validator.hasDataset("detector_index", {"mantid_workspace_1", rootName, "bank1"}));
      TS_ASSERT(validator.hasDataset("detector_count", {"mantid_workspace_1", rootName, "bank1"}));
      TS_ASSERT(validator.hasDataset("spectra", {"mantid_workspace_1", rootName, "bank2"}));
      TS_ASSERT(validator.hasDataset("detector_list", {"mantid_workspace_1", rootName, "bank2"}));
      TS_ASSERT(validator.hasDataset("detector_index", {"mantid_workspace_1", rootName, "bank2"}));
      TS_ASSERT(validator.hasDataset("detector_count", {"mantid_workspace_1", rootName, "bank2"}));
    }
  }

  void test_base_function_with_workspace() {

    // This is testing the core routine, but we put it here and not in
    // NexusGeometrySave because we need access to WorkspaceCreationHelpers for
    // this.
    FileResource fileResource("test_with_full_workspace.hdf5");
    std::string destinationFile = fileResource.fullPath();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20);
    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);
    Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(*ws, destinationFile, "entry", adapter);
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
};
