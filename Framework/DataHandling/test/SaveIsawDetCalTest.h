// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/SaveIsawDetCal.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using Mantid::Kernel::V3D;

namespace {
/// Read the panel centre, in centimetres, from the "5" record of the given bank.
V3D readBankCentre(const std::string &filename, const int bank) {
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream stream(line);
    std::vector<std::string> fields{std::istream_iterator<std::string>(stream), {}};
    // 5 bank nrows ncols width height depth detd centerX centerY centerZ baseX ...
    if (fields.size() > 10 && fields[0] == "5" && std::stoi(fields[1]) == bank)
      return V3D(std::stod(fields[8]), std::stod(fields[9]), std::stod(fields[10]));
  }
  throw std::runtime_error("No record for bank " + std::to_string(bank) + " in " + filename);
}
} // namespace

class SaveIsawDetCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveIsawDetCalTest *createSuite() { return new SaveIsawDetCalTest(); }
  static void destroySuite(SaveIsawDetCalTest *suite) { delete suite; }

  void test_Init() {
    SaveIsawDetCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 50);

    SaveIsawDetCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "SaveIsawDetCalTest.DetCal"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(ws)));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(std::filesystem::exists(filename));
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
  }

  /** Regression test: geometry held in the ParameterMap must reach the written file.
   *
   * On CORELLI the panel is the "sixteenpack" child of bankNN, which the algorithm reaches via
   * ComponentInfo::componentID(). Dereferencing that ID yields the *base* component, whose getPos()
   * ignores the ParameterMap, so a calibrated panel was previously written at its engineering position.
   */
  void test_corelli_panel_written_from_calibrated_geometry() {
    LoadEmptyInstrument loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("InstrumentName", "CORELLI");
    loader.setPropertyValue("OutputWorkspace", "_unused_");
    loader.execute();
    MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");
    TS_ASSERT(ws);

    // Displace the panel through ComponentInfo, which is how a calibration records its result.
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t bankIndex = componentInfo.indexOfAny("bank21");
    const size_t panelIndex = componentInfo.children(bankIndex).front();
    const V3D pristine = componentInfo.position(panelIndex);
    const V3D displaced = pristine + V3D(0.005, 0.0, 0.0);
    componentInfo.setPosition(panelIndex, displaced);

    SaveIsawDetCal alg;
    alg.initialize();
    alg.setPropertyValue("Filename", "SaveIsawDetCalTest_corelli.DetCal");
    alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(ws));
    alg.setPropertyValue("BankNames", "bank21");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const std::string filename = alg.getPropertyValue("Filename");
    const V3D centre = readBankCentre(filename, 21);

    // The file stores centimetres to four decimal places.
    TS_ASSERT_DELTA(centre.X(), 100.0 * displaced.X(), 1e-3);
    TS_ASSERT_DELTA(centre.Y(), 100.0 * displaced.Y(), 1e-3);
    TS_ASSERT_DELTA(centre.Z(), 100.0 * displaced.Z(), 1e-3);
    // Guard against the assertion above passing trivially: the displacement must be observable.
    TS_ASSERT_DELTA(centre.X() - 100.0 * pristine.X(), 0.5, 1e-3);

    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
  }
};
