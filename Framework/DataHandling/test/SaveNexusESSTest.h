// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSESSTEST_H_
#define MANTID_DATAHANDLING_SAVENEXUSESSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveNexusESS.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidTestHelpers/FileResource.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/filesystem.hpp>
#include <memory>
using Mantid::DataHandling::SaveNexusESS;

class SaveNexusESSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNexusESSTest *createSuite() { return new SaveNexusESSTest(); }
  static void destroySuite(SaveNexusESSTest *suite) { delete suite; }

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

  void test_Init() {
    SaveNexusESS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_instrument_details() {
    using namespace Mantid::NexusGeometry;
    const ScopedFileHandle fileInfo("test_rectangular.nxs");
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
    TS_ASSERT_EQUALS(outCompInfo->size() - 10, inCompInfo.size());
  }
};

#endif /* MANTID_DATAHANDLING_SAVENEXUSESSTEST_H_ */
