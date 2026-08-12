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
#include <string>
#include <vector>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

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

    std::string outfile = "./SaveIsawPeaksTest.peaks";
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

  /** Regression test: geometry held in the ParameterMap must reach the written file.
   *
   * On CORELLI the panel is the "sixteenpack" child of bankNN, which the algorithm reaches via
   * ComponentInfo::componentID(). Dereferencing that ID yields the *base* component, whose getPos()
   * ignores the ParameterMap, so a calibrated panel was previously written at its engineering position.
   */
  void test_corelli_panel_written_from_calibrated_geometry() {
    FrameworkManager::Instance().exec("LoadEmptyInstrument", 4, "InstrumentName", "CORELLI", "OutputWorkspace",
                                      "_corelli_inst_");
    MatrixWorkspace_sptr inst_ws;
    TS_ASSERT_THROWS_NOTHING(inst_ws = std::dynamic_pointer_cast<MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("_corelli_inst_")))
    TS_ASSERT(inst_ws)

    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst_ws->getInstrument());

    // One peak anywhere in bank21, so that the algorithm emits a record for that bank.
    const size_t bankIndex = ws->componentInfo().indexOfAny("bank21");
    const auto detectorsInBank = ws->componentInfo().detectorsInSubtree(bankIndex);
    TS_ASSERT(!detectorsInBank.empty())
    const auto detID = ws->detectorInfo().detectorIDs()[detectorsInBank.front()];
    ws->addPeak(Peak(ws->getInstrument(), detID, 1.5));

    // Displace the panel through ComponentInfo, which is how a calibration records its result.
    auto &componentInfo = ws->mutableComponentInfo();
    const size_t panelIndex = componentInfo.children(bankIndex).front();
    const V3D pristine = componentInfo.position(panelIndex);
    const V3D displaced = pristine + V3D(0.005, 0.0, 0.0);
    componentInfo.setPosition(panelIndex, displaced);

    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "./SaveIsawPeaksTest_corelli.peaks"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    const std::string outfile = alg.getPropertyValue("Filename");
    // Locate the "5" record for bank 21 and read its centre, stored in centimetres.
    std::ifstream in(outfile.c_str());
    std::string line;
    bool found(false);
    while (std::getline(in, line)) {
      std::istringstream stream(line);
      std::vector<std::string> fields{std::istream_iterator<std::string>(stream), {}};
      if (fields.size() > 10 && fields[0] == "5" && std::stoi(fields[1]) == 21) {
        found = true;
        TS_ASSERT_DELTA(std::stod(fields[8]), 100.0 * displaced.X(), 1e-3);
        TS_ASSERT_DELTA(std::stod(fields[9]), 100.0 * displaced.Y(), 1e-3);
        TS_ASSERT_DELTA(std::stod(fields[10]), 100.0 * displaced.Z(), 1e-3);
        // Guard against the assertions above passing trivially.
        TS_ASSERT_DELTA(std::stod(fields[8]) - 100.0 * pristine.X(), 0.5, 1e-3);
        break;
      }
    }
    in.close();
    TS_ASSERT(found)

    if (std::filesystem::exists(outfile))
      std::filesystem::remove(outfile);
    AnalysisDataService::Instance().remove("_corelli_inst_");
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
    std::string outfile = "./SaveIsawPeaksTest.peaks";
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
  }
};
