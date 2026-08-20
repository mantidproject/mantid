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
#include "MantidKernel/Quat.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

namespace {
/// Absolute path in the system temporary directory, carrying a random token so that concurrent runs, and
/// files orphaned by a failed test, cannot collide. The runner's cwd is never written to.
std::string tempPath(const std::string &filename) {
  return (std::filesystem::temp_directory_path() / (Mantid::Kernel::Strings::randomString(8) + "_" + filename))
      .string();
}

/** Return the fields of the "5" record written for the given bank.
 *
 * The record layout is
 *   5 bank nrows ncols width height depth detd centerX centerY centerZ baseX baseY baseZ upX upY upZ
 * so the indices used below are 4 and 5 for the panel extents, 8 to 10 for the centre, 11 to 13 for the
 * base vector and 14 to 16 for the up vector. Lengths are in centimetres.
 */
std::vector<std::string> readBankRecord(const std::string &filename, const int bank) {
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream stream(line);
    std::vector<std::string> fields{std::istream_iterator<std::string>(stream), {}};
    if (fields.size() > 16 && fields[0] == "5" && std::stoi(fields[1]) == bank)
      return fields;
  }
  throw std::runtime_error("No record for bank " + std::to_string(bank) + " in " + filename);
}

/// Panel centre, in centimetres.
V3D readBankCentre(const std::string &filename, const int bank) {
  const auto fields = readBankRecord(filename, bank);
  return V3D(std::stod(fields[8]), std::stod(fields[9]), std::stod(fields[10]));
}

/// Panel width and height, in centimetres.
std::pair<double, double> readBankExtents(const std::string &filename, const int bank) {
  const auto fields = readBankRecord(filename, bank);
  return {std::stod(fields[4]), std::stod(fields[5])};
}

/// Unit vectors along the panel's horizontal (base) and vertical (up) directions.
std::pair<V3D, V3D> readBankOrientation(const std::string &filename, const int bank) {
  const auto fields = readBankRecord(filename, bank);
  return {V3D(std::stod(fields[11]), std::stod(fields[12]), std::stod(fields[13])),
          V3D(std::stod(fields[14]), std::stod(fields[15]), std::stod(fields[16]))};
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

  /** Regression test for the panel centre.
   *
   * Asserts that the centre recorded for a CORELLI panel (fields 9 to 11 of the "5" record) follows a
   * displacement applied through ComponentInfo, rather than reporting the position in the instrument
   * definition.
   *
   * Guards the panel-centre lookup in SaveIsawDetCal::exec. On CORELLI the panel is the "sixteenpack"
   * child of bankNN, reached via ComponentInfo::componentID(). Dereferencing that ID yields the *base*
   * component, whose getPos() does not consult the ParameterMap, so a calibrated panel was written at
   * its engineering position. A whole-panel displacement is the right perturbation here because the
   * centre is exactly what it changes.
   */
  void test_corelli_panel_centre_written_from_calibrated_geometry() {
    MatrixWorkspace_sptr ws = loadCorelli();
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t panelIndex = corelliPanelIndex(componentInfo);

    // Displace the panel through ComponentInfo, which is how a calibration records its result.
    const V3D pristine = componentInfo.position(panelIndex);
    const V3D displaced = pristine + V3D(0.005, 0.0, 0.0);
    componentInfo.setPosition(panelIndex, displaced);

    const std::string filename = saveBank21(ws, tempPath("SaveIsawDetCalTest_corelli_centre.DetCal"));
    const V3D centre = readBankCentre(filename, 21);

    // The file stores centimetres to four decimal places.
    TS_ASSERT_DELTA(centre.X(), 100.0 * displaced.X(), 1e-3);
    TS_ASSERT_DELTA(centre.Y(), 100.0 * displaced.Y(), 1e-3);
    TS_ASSERT_DELTA(centre.Z(), 100.0 * displaced.Z(), 1e-3);
    // Guard against the assertions above passing trivially: the displacement must be observable.
    TS_ASSERT_DELTA(centre.X() - 100.0 * pristine.X(), 0.5, 1e-3);

    removeIfPresent(filename);
  }

  /** Regression test for the panel extents.
   *
   * Asserts that the width and height recorded for a CORELLI panel (fields 5 and 6 of the "5" record)
   * follow displacements of individual detector components applied through ComponentInfo.
   *
   * Guards SaveIsawDetCal::sizeBanks, which measured the extents by dereferencing
   * ComponentInfo::componentID() and so reported the spacing given in the instrument definition.
   *
   * Note the perturbation: sizeBanks measures the span between the first and last tube of the pack, and
   * between the first and last pixel of the first tube. Those distances are invariant under any rigid
   * motion of the whole panel, so a panel-level displacement or rotation cannot detect this defect. A
   * single tube and a single pixel must be moved instead. Simplifying this test to move the panel would
   * leave it passing regardless of the code under test.
   */
  void test_corelli_panel_extents_written_from_calibrated_geometry() {
    MatrixWorkspace_sptr ws = loadCorelli();
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t panelIndex = corelliPanelIndex(componentInfo);

    const auto tubes = componentInfo.children(panelIndex);
    const auto pixels = componentInfo.children(tubes.front());
    const double pristineWidth = componentInfo.position(tubes.front()).distance(componentInfo.position(tubes.back()));
    const double pristineHeight =
        componentInfo.position(pixels.front()).distance(componentInfo.position(pixels.back()));

    // Widen the pack by 1 cm by moving its last tube along the tube-spacing direction, and lengthen the
    // first tube by 1 cm by moving its last pixel along the pixel direction.
    V3D acrossTubes = componentInfo.position(tubes.back()) - componentInfo.position(tubes.front());
    acrossTubes.normalize();
    componentInfo.setPosition(tubes.back(), componentInfo.position(tubes.back()) + acrossTubes * 0.01);
    V3D alongTube = componentInfo.position(pixels.back()) - componentInfo.position(pixels.front());
    alongTube.normalize();
    componentInfo.setPosition(pixels.back(), componentInfo.position(pixels.back()) + alongTube * 0.01);

    const std::string filename = saveBank21(ws, tempPath("SaveIsawDetCalTest_corelli_extents.DetCal"));
    const auto [width, height] = readBankExtents(filename, 21);

    const double expectedWidth = componentInfo.position(tubes.front()).distance(componentInfo.position(tubes.back()));
    const double expectedHeight =
        componentInfo.position(pixels.front()).distance(componentInfo.position(pixels.back()));
    TS_ASSERT_DELTA(width, 100.0 * expectedWidth, 1e-3);
    TS_ASSERT_DELTA(height, 100.0 * expectedHeight, 1e-3);
    // Guard: each extent must have grown by the 1 cm applied above.
    TS_ASSERT_DELTA(width - 100.0 * pristineWidth, 1.0, 1e-3);
    TS_ASSERT_DELTA(height - 100.0 * pristineHeight, 1.0, 1e-3);

    removeIfPresent(filename);
  }

  /** Test for the panel orientation.
   *
   * Asserts that the base and up vectors recorded for a CORELLI panel (fields 12 to 14 and 15 to 17 of
   * the "5" record) follow a rotation applied through ComponentInfo. Because the panel is rigid, both
   * vectors must equal their pristine values with the same rotation applied.
   *
   * This covers SaveIsawDetCal::findPixelPos, which supplies the pixel positions the two vectors are
   * derived from. Unlike the two tests above, it locks in behaviour that was already correct: that
   * function reads ComponentInfo::position and was not among the defects fixed alongside this test. It
   * is included because the same function in SaveIsawPeaks did carry the defect.
   *
   * Note the perturbation: base and up are normalised differences between pixel positions, so they are
   * invariant under translation. Only a rotation exercises them.
   */
  void test_corelli_panel_orientation_written_from_calibrated_geometry() {
    MatrixWorkspace_sptr ws = loadCorelli();
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t panelIndex = corelliPanelIndex(componentInfo);

    const std::string before = saveBank21(ws, tempPath("SaveIsawDetCalTest_corelli_orient_before.DetCal"));
    const auto [pristineBase, pristineUp] = readBankOrientation(before, 21);

    // Rotate about the panel's own centre; setRotation takes an absolute rotation.
    const Quat extra(5.0, V3D(0.0, 0.0, 1.0));
    componentInfo.setRotation(panelIndex, extra * componentInfo.rotation(panelIndex));

    const std::string after = saveBank21(ws, tempPath("SaveIsawDetCalTest_corelli_orient_after.DetCal"));
    const auto [rotatedBase, rotatedUp] = readBankOrientation(after, 21);

    V3D expectedBase = pristineBase;
    V3D expectedUp = pristineUp;
    extra.rotate(expectedBase);
    extra.rotate(expectedUp);
    // The file stores the unit vectors to five decimal places.
    for (size_t i = 0; i < 3; ++i) {
      TS_ASSERT_DELTA(rotatedBase[i], expectedBase[i], 1e-4);
      TS_ASSERT_DELTA(rotatedUp[i], expectedUp[i], 1e-4);
    }
    // Guard against the assertions above passing trivially: the rotation must be observable.
    TS_ASSERT(rotatedBase.distance(pristineBase) > 1e-3);

    removeIfPresent(before);
    removeIfPresent(after);
  }

private:
  /// Load a pristine CORELLI instrument into a workspace.
  MatrixWorkspace_sptr loadCorelli() {
    LoadEmptyInstrument loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("InstrumentName", "CORELLI");
    loader.setPropertyValue("OutputWorkspace", "_unused_");
    loader.execute();
    MatrixWorkspace_sptr ws = loader.getProperty("OutputWorkspace");
    TS_ASSERT(ws);
    return ws;
  }

  /// Index of the "sixteenpack" panel under bank21, which is the component CORELLI calibrations adjust.
  size_t corelliPanelIndex(const Geometry::ComponentInfo &componentInfo) {
    return componentInfo.children(componentInfo.indexOfAny("bank21")).front();
  }

  /// Write bank21 only, and return the path of the file produced.
  std::string saveBank21(const MatrixWorkspace_sptr &ws, const std::string &filename) {
    SaveIsawDetCal alg;
    alg.initialize();
    alg.setPropertyValue("Filename", filename);
    alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(ws));
    alg.setPropertyValue("BankNames", "bank21");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    return alg.getPropertyValue("Filename");
  }

  void removeIfPresent(const std::string &filename) {
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
  }
};
