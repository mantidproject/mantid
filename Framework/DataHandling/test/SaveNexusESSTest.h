// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSESSTEST_H_
#define MANTID_DATAHANDLING_SAVENEXUSESSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/SaveNexusESS.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/FileResource.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/filesystem.hpp>
#include <memory>

using namespace Mantid::DataHandling;
using namespace Mantid::API;

template <typename T> void do_execute(const std::string filename, T &ws) {
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

template <typename T> void do_execute2(const std::string filename, T &ws) {
  SaveNexusProcessed alg;
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
  LoadNexusProcessed loader;
  loader.setChild(true);
  loader.setRethrows(true);
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  Workspace_sptr out = loader.getProperty("OutputWorkspace");
  auto matrixWSOut =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(out);
  return matrixWSOut;
}

Mantid::API::MatrixWorkspace_sptr
from_instrument_file(const std::string &filename) {
  LoadEmptyInstrument loader;
  loader.setChild(true);
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", "dummy");
  loader.execute();
  MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");
  return ws;
}
} // namespace test_utility

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
    ScopedFileHandle fileInfo("test_rectangular_instrument.nxs");
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1 /*numBanks*/, 10 /*numPixels*/, 10 /*numBins*/);

    const auto &inDetInfo = ws->detectorInfo();
    const auto &inCompInfo = ws->componentInfo();

    do_execute(fileInfo.fullPath(), ws);

    // Load and check instrument geometry
    Mantid::Kernel::Logger logger("test_logger");
    auto instr = NexusGeometryParser::createInstrument(fileInfo.fullPath(),
                                                       makeLogger(&logger));
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
    ScopedFileHandle fileInfo("test_rectangular_data.nxs");
    auto wsIn =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1 /*numBanks*/, 10 /*numPixels*/, 12 /*numBins*/);

    do_execute(fileInfo.fullPath(), wsIn);

    auto matrixWSOut = test_utility::reload(fileInfo.fullPath());

    TS_ASSERT_EQUALS(matrixWSOut->blocksize(), 12);
    TS_ASSERT_EQUALS(matrixWSOut->getNumberHistograms(), 10 * 10);
    TS_ASSERT(matrixWSOut->detectorInfo().isEquivalent(wsIn->detectorInfo()));
  }

  void test_with_ess_instrument() {

    using namespace Mantid::HistogramData;

    auto wsIn = test_utility::from_instrument_file("SXD_Definition.xml");

    auto &compInfo = wsIn->mutableComponentInfo();
    compInfo.setPosition(compInfo.sample(), Mantid::Kernel::V3D(0, 0, 0));

    ScopedFileHandle fileInfo("test_ess_instrument_new.nxs");
    do_execute(fileInfo.fullPath(), wsIn);

    ScopedFileHandle fileInfo1("test_ess_instrument_old.nxs");
    do_execute2(fileInfo1.fullPath(), wsIn);
  }

  // Characterise behaviour to fix later. Issue here is that LoadNexusProcessed
  // expects a single NXDetector called "Detector". That is not thw way that we
  // record the instrument
  void test_demonstrate_no_spectra_detector_map_loaded() {
    using namespace Mantid::Indexing;
    ScopedFileHandle fileInfo("test_no_spectra_mapping_ess.nxs");
    fileInfo.setDebugMode(true);
    auto wsIn =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2 /*numBanks*/, 10 /*numPixels*/, 12 /*numBins*/);

    std::vector<SpectrumDefinition> specDefinitions;
    std::vector<SpectrumNumber> spectrumNumbers;
    size_t i = wsIn->getNumberHistograms() - 1;
    for (size_t j = 0; j < wsIn->getNumberHistograms(); --i, ++j) {
      specDefinitions.push_back(SpectrumDefinition(i));
      spectrumNumbers.push_back(SpectrumNumber(static_cast<int>(j)));
    }
    IndexInfo info(spectrumNumbers);
    info.setSpectrumDefinitions(specDefinitions);
    wsIn->setIndexInfo(info);
    do_execute(fileInfo.fullPath(), wsIn);

    // Reload it.
    auto matrixWSOut = test_utility::reload(fileInfo.fullPath());

    const auto &inSpecInfo = wsIn->spectrumInfo();
    const auto &outSpecInfo = matrixWSOut->spectrumInfo();
    TS_ASSERT_EQUALS(outSpecInfo.size(), inSpecInfo.size());
    for (size_t i = 0; i < outSpecInfo.size(); ++i) {
      // Output has no mapping, so for each spectrum have 0 detector indices
      TS_ASSERT_EQUALS(outSpecInfo.spectrumDefinition(i).size() + 1,
                       inSpecInfo.spectrumDefinition(i).size());
      // Compare actual detector indices for each spectrum when fixed as below
      // TS_ASSERT_EQUALS(outSpecInfo.spectrumDefinition(i)[0],
      //                 inSpecInfo.spectrumDefinition(i)[0]);
    }
  }
  void test_base_function_with_workspace() {

    // This is testing the core routine, but we put it here and not in
    // NexusGeometrySave because we need access to WorkspaceCreationHelpers for
    // this.
    ScopedFileHandle fileResource("test_with_full_workspace.hdf5");
    std::string destinationFile = fileResource.fullPath();
    // auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
    //    10 /*histograms*/, 100 /*bins*/);
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2, 10, 20);
    Mantid::Kernel::Logger logger("logger");
    Mantid::NexusGeometry::LogAdapter<Mantid::Kernel::Logger> adapter(&logger);
    Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(
        *ws, destinationFile, "entry", adapter);
  }
};

#endif /* MANTID_DATAHANDLING_SAVENEXUSESSTEST_H_ */
