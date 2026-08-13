// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
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
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace {
/// Absolute path in the system temporary directory, so tests never write into the runner's cwd.
std::string tempPath(const std::string &filename) {
  return (std::filesystem::temp_directory_path() / filename).string();
}

/** Return the fields of the "5" record written for the given bank.
 *
 * The record layout is
 *   5 bank nrows ncols width height depth detd centerX centerY centerZ baseX baseY baseZ upX upY upZ
 * so the indices used below are 4 and 5 for the panel extents, 8 to 10 for the centre, 11 to 13 for the
 * base vector and 14 to 16 for the up vector. Lengths are in centimetres.
 */
std::vector<std::string> readPeaksBankRecord(const std::string &filename, const int bank) {
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
V3D readPeaksBankCentre(const std::string &filename, const int bank) {
  const auto fields = readPeaksBankRecord(filename, bank);
  return V3D(std::stod(fields[8]), std::stod(fields[9]), std::stod(fields[10]));
}

/// Panel width and height, in centimetres.
std::pair<double, double> readPeaksBankExtents(const std::string &filename, const int bank) {
  const auto fields = readPeaksBankRecord(filename, bank);
  return {std::stod(fields[4]), std::stod(fields[5])};
}

/// Unit vectors along the panel's horizontal (base) and vertical (up) directions.
std::pair<V3D, V3D> readPeaksBankOrientation(const std::string &filename, const int bank) {
  const auto fields = readPeaksBankRecord(filename, bank);
  return {V3D(std::stod(fields[11]), std::stod(fields[12]), std::stod(fields[13])),
          V3D(std::stod(fields[14]), std::stod(fields[15]), std::stod(fields[16]))};
}
} // namespace

class SaveIsawPeaksTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank) {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);

    for (int run = 1000; run < numRuns + 1000; run++)
      for (size_t b = 1; b <= numBanks; b++)
        for (size_t i = 0; i < numPeaksPerBank; i++) {
          V3D hkl(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
          DblMatrix gon(3, 3, true);
          Peak p(inst, static_cast<detid_t>(b * 100 + i + 1 + i * 10), static_cast<double>(i) * 1.0 + 0.5, hkl, gon);
          p.setRunNumber(run);
          p.setIntensity(static_cast<double>(i) + 0.1);
          p.setSigmaIntensity(sqrt(static_cast<double>(i)));
          p.setBinCount(static_cast<double>(i));
          p.setPeakNumber((run - 1000) * static_cast<int>(numBanks * numPeaksPerBank) +
                          static_cast<int>(b * numPeaksPerBank + i));
          ws->addPeak(p);
        }

    std::string outfile = tempPath("SaveIsawPeaksTest.peaks");
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    // Test appending same file to check peak numbers
    SaveIsawPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("Filename", outfile))
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("AppendFile", true))
    TS_ASSERT_THROWS_NOTHING(alg2.execute();)

    // Get the file
    if (numPeaksPerBank > 0) {
      outfile = alg2.getPropertyValue("Filename");
      TS_ASSERT(std::filesystem::exists(outfile))
      std::ifstream in(outfile.c_str());
      std::string line0;
      getline(in, line0);
      TS_ASSERT_EQUALS(line0, "Version: 2.0  Facility: Unknown  Instrument: "
                              "basic_rect  Date: 1990-01-01T00:00:01")
      std::string line;
      while (!in.eof()) // To get you all the lines.
      {
        getline(in, line0); // Saves the line in STRING.
        if (in.eof())
          break;
        line = line0;
      }
      TS_ASSERT_EQUALS(line, "3     71   -3   -3   -3    3.00     4.00    "
                             "27086  2061.553   0.24498   0.92730   3.500000   "
                             "14.3227        3       3.10    1.73   310")
    }

    if (std::filesystem::exists(outfile))
      std::filesystem::remove(outfile);
  }

  /** Regression test for the panel centre.
   *
   * Asserts that the centre recorded for a CORELLI panel (fields 9 to 11 of the "5" record) follows a
   * displacement applied through ComponentInfo, rather than reporting the position in the instrument
   * definition.
   *
   * Guards the panel-centre lookup in SaveIsawPeaks::exec. On CORELLI the panel is the "sixteenpack"
   * child of bankNN, reached via ComponentInfo::componentID(). Dereferencing that ID yields the *base*
   * component, whose getPos() does not consult the ParameterMap, so a calibrated panel was written at
   * its engineering position. A whole-panel displacement is the right perturbation here because the
   * centre is exactly what it changes.
   */
  void test_corelli_panel_centre_written_from_calibrated_geometry() {
    PeaksWorkspace_sptr ws = makeCorelliPeaksWorkspace();
    TS_ASSERT(ws)
    if (!ws)
      return;
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t panelIndex = corelliPanelIndex(componentInfo);

    // Displace the panel through ComponentInfo, which is how a calibration records its result.
    const V3D pristine = componentInfo.position(panelIndex);
    const V3D displaced = pristine + V3D(0.005, 0.0, 0.0);
    componentInfo.setPosition(panelIndex, displaced);

    const std::string outfile = savePeaks(ws, tempPath("SaveIsawPeaksTest_corelli_centre.peaks"));
    const V3D centre = readPeaksBankCentre(outfile, 21);

    // The file stores centimetres to four decimal places.
    TS_ASSERT_DELTA(centre.X(), 100.0 * displaced.X(), 1e-3);
    TS_ASSERT_DELTA(centre.Y(), 100.0 * displaced.Y(), 1e-3);
    TS_ASSERT_DELTA(centre.Z(), 100.0 * displaced.Z(), 1e-3);
    // Guard against the assertions above passing trivially: the displacement must be observable.
    TS_ASSERT_DELTA(centre.X() - 100.0 * pristine.X(), 0.5, 1e-3);

    removeIfPresent(outfile);
  }

  /** Regression test for the panel extents.
   *
   * Asserts that the width and height recorded for a CORELLI panel (fields 5 and 6 of the "5" record)
   * follow displacements of individual detector components applied through ComponentInfo.
   *
   * Guards SaveIsawPeaks::sizeBanks, which measured the extents by dereferencing
   * ComponentInfo::componentID() and so reported the spacing given in the instrument definition.
   *
   * Note the perturbation: sizeBanks measures the span between the first and last tube of the pack, and
   * between the first and last pixel of the first tube. Those distances are invariant under any rigid
   * motion of the whole panel, so a panel-level displacement or rotation cannot detect this defect. A
   * single tube and a single pixel must be moved instead. Simplifying this test to move the panel would
   * leave it passing regardless of the code under test.
   */
  void test_corelli_panel_extents_written_from_calibrated_geometry() {
    PeaksWorkspace_sptr ws = makeCorelliPeaksWorkspace();
    TS_ASSERT(ws)
    if (!ws)
      return;
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

    const std::string outfile = savePeaks(ws, tempPath("SaveIsawPeaksTest_corelli_extents.peaks"));
    const auto [width, height] = readPeaksBankExtents(outfile, 21);

    const double expectedWidth = componentInfo.position(tubes.front()).distance(componentInfo.position(tubes.back()));
    const double expectedHeight =
        componentInfo.position(pixels.front()).distance(componentInfo.position(pixels.back()));
    TS_ASSERT_DELTA(width, 100.0 * expectedWidth, 1e-3);
    TS_ASSERT_DELTA(height, 100.0 * expectedHeight, 1e-3);
    // Guard: each extent must have grown by the 1 cm applied above.
    TS_ASSERT_DELTA(width - 100.0 * pristineWidth, 1.0, 1e-3);
    TS_ASSERT_DELTA(height - 100.0 * pristineHeight, 1.0, 1e-3);

    removeIfPresent(outfile);
  }

  /** Regression test for the panel orientation.
   *
   * Asserts that the base and up vectors recorded for a CORELLI panel (fields 12 to 14 and 15 to 17 of
   * the "5" record) follow a rotation applied through ComponentInfo. Because the panel is rigid, both
   * vectors must equal their pristine values with the same rotation applied.
   *
   * Guards SaveIsawPeaks::findPixelPos, which supplies the pixel positions the two vectors are derived
   * from. It read them by dereferencing ComponentInfo::componentID(), so a calibrated panel was recorded
   * with the orientation given in the instrument definition. This is the reason the recorded orientation,
   * and not only the recorded position, was wrong before the fix.
   *
   * Note the perturbation: base and up are normalised differences between pixel positions, so they are
   * invariant under translation. Only a rotation exercises them.
   */
  void test_corelli_panel_orientation_written_from_calibrated_geometry() {
    PeaksWorkspace_sptr ws = makeCorelliPeaksWorkspace();
    TS_ASSERT(ws)
    if (!ws)
      return;
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t panelIndex = corelliPanelIndex(componentInfo);

    const std::string before = savePeaks(ws, tempPath("SaveIsawPeaksTest_corelli_orient_before.peaks"));
    const auto [pristineBase, pristineUp] = readPeaksBankOrientation(before, 21);

    // Rotate about the panel's own centre; setRotation takes an absolute rotation.
    const Quat extra(5.0, V3D(0.0, 0.0, 1.0));
    componentInfo.setRotation(panelIndex, extra * componentInfo.rotation(panelIndex));

    const std::string after = savePeaks(ws, tempPath("SaveIsawPeaksTest_corelli_orient_after.peaks"));
    const auto [rotatedBase, rotatedUp] = readPeaksBankOrientation(after, 21);

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

  /// Test with an empty PeaksWorkspace
  void test_empty() { do_test(0, 0, 0); }

  /// Test with a few peaks
  void test_exec() { do_test(2, 4, 4); }

  void test_mod() {
    LoadIsawPeaks alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    alg1.setPropertyValue("Filename", "Modulated.peaks");
    alg1.setPropertyValue("OutputWorkspace", "peaks");

    TS_ASSERT(alg1.execute())
    TS_ASSERT(alg1.isExecuted())

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("peaks")));
    TS_ASSERT(ws)
    if (!ws)
      return;
    std::string outfile = tempPath("SaveIsawPeaksTest_mod.peaks");
    SaveIsawPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("Filename", outfile))
    TS_ASSERT_THROWS_NOTHING(alg2.execute();)
    LoadIsawPeaks alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize())
    TS_ASSERT(alg3.isInitialized())
    alg3.setPropertyValue("Filename", outfile);
    alg3.setPropertyValue("OutputWorkspace", "peaks2");

    TS_ASSERT(alg3.execute())
    TS_ASSERT(alg3.isExecuted())

    PeaksWorkspace_sptr ws2;
    TS_ASSERT_THROWS_NOTHING(
        ws2 = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("peaks2")))
    TS_ASSERT(ws2)
    if (!ws2)
      return;
    TS_ASSERT_EQUALS(ws2->getNumberPeaks(), 18)

    Peak p = ws->getPeaks()[0];
    Peak p2 = ws2->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getRunNumber(), p2.getRunNumber())
    TS_ASSERT_DELTA(p.getH(), p2.getH(), 1e-4)
    TS_ASSERT_DELTA(p.getK(), p2.getK(), 1e-4)
    TS_ASSERT_DELTA(p.getL(), p2.getL(), 1e-4)
    TS_ASSERT_EQUALS(p.getIntMNP(), p2.getIntMNP())
    TS_ASSERT_EQUALS(p.getBankName(), p2.getBankName())
    TS_ASSERT_DELTA(p.getCol(), p2.getCol(), 1e-4)
    TS_ASSERT_DELTA(p.getRow(), p2.getRow(), 1e-4)
    TS_ASSERT_DELTA(p.getIntensity(), p2.getIntensity(), 0.01)
    TS_ASSERT_DELTA(p.getSigmaIntensity(), p2.getSigmaIntensity(), 0.01)
    TS_ASSERT_DELTA(p.getBinCount(), p2.getBinCount(), 1)
    TS_ASSERT_DELTA(p.getWavelength(), p2.getWavelength(), 0.001)
    TS_ASSERT_DELTA(p.getL1(), p2.getL1(), 1e-3)
    TS_ASSERT_DELTA(p.getL2(), p2.getL2(), 1e-3)

    // channel number is about TOF
    TS_ASSERT_DELTA(p.getTOF(), p2.getTOF(), 0.1)
    TS_ASSERT_DELTA(p.getDSpacing(), p2.getDSpacing(), 0.001)

    removeIfPresent(outfile);
  }

private:
  /// A PeaksWorkspace on a pristine CORELLI instrument, holding one peak in bank21 so that the
  /// algorithm emits a "5" record for that bank.
  PeaksWorkspace_sptr makeCorelliPeaksWorkspace() {
    FrameworkManager::Instance().exec("LoadEmptyInstrument", 4, "InstrumentName", "CORELLI", "OutputWorkspace",
                                      "_corelli_inst_");
    MatrixWorkspace_sptr inst_ws;
    TS_ASSERT_THROWS_NOTHING(inst_ws = std::dynamic_pointer_cast<MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("_corelli_inst_")))
    TS_ASSERT(inst_ws)
    if (!inst_ws)
      return PeaksWorkspace_sptr();

    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst_ws->getInstrument());

    const size_t bankIndex = ws->componentInfo().indexOfAny("bank21");
    const auto detectorsInBank = ws->componentInfo().detectorsInSubtree(bankIndex);
    TS_ASSERT(!detectorsInBank.empty())
    const auto detID = ws->detectorInfo().detectorIDs()[detectorsInBank.front()];
    ws->addPeak(Peak(ws->getInstrument(), detID, 1.5));

    AnalysisDataService::Instance().remove("_corelli_inst_");
    return ws;
  }

  /// Index of the "sixteenpack" panel under bank21, which is the component CORELLI calibrations adjust.
  size_t corelliPanelIndex(const Geometry::ComponentInfo &componentInfo) {
    return componentInfo.children(componentInfo.indexOfAny("bank21")).front();
  }

  /// Write the peaks file and return the path produced.
  std::string savePeaks(const PeaksWorkspace_sptr &ws, const std::string &filename) {
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    return alg.getPropertyValue("Filename");
  }

  void removeIfPresent(const std::string &filename) {
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
  }
};
