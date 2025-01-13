// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/SaveLauenorm.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Timer.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::PhysicalConstants;

class SaveLauenormTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveLauenorm alg;
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
          p.setBankName("bank1");
          p.setIntensity(static_cast<double>(i) + 0.1);
          p.setSigmaIntensity(sqrt(static_cast<double>(i)));
          p.setBinCount(static_cast<double>(i));
          ws->addPeak(p);
        }

    std::string outfile = "./LAUE";
    SaveLauenorm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Get the file
    outfile = alg.getPropertyValue("Filename") + "001";
    bool fileExists = false;
    TS_ASSERT(fileExists = Poco::File(outfile).exists());

    std::ifstream in(outfile.c_str());

    double d1, d2, d3, d4, d5, d6, d7;
    if (numPeaksPerBank > 0) {
      in >> d1 >> d2 >> d3 >> d4 >> d5 >> d6 >> d7;
      TS_ASSERT_EQUALS(d1, -1);
      TS_ASSERT_EQUALS(d2, -1);
      TS_ASSERT_EQUALS(d3, -1);
      TS_ASSERT_EQUALS(d6, 1);
      TS_ASSERT_EQUALS(d7, 1);
      TS_ASSERT_EQUALS(d4, 1.5);
      TS_ASSERT_DELTA(d5, 0.21025, 1e-4);
    }

    std::string outfile2 = "./LAUE2";

    // Now try with setting detector parameter
    auto &paramMap = ws->instrumentParameters();
    paramMap.addDouble(inst.get(), "detScale1", 0.5);

    SaveLauenorm alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("Filename", outfile2));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("UseDetScale", true));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());
    // Get the file
    outfile2 = alg2.getPropertyValue("Filename") + "001";
    TS_ASSERT(Poco::File(outfile2).exists());

    std::ifstream in2(outfile2.c_str());

    if (numPeaksPerBank > 0) {
      for (int i = 0; i < 17; i++)
        in2 >> d1 >> d2 >> d3 >> d4 >> d5 >> d6 >> d7;
      TS_ASSERT_EQUALS(d1, -3);
      TS_ASSERT_EQUALS(d2, -3);
      TS_ASSERT_EQUALS(d3, -3);
      TS_ASSERT_EQUALS(d4, 3.5);
      TS_ASSERT_DELTA(d5, 0.39270, 1e-4);
      // was d6=3; d7=2 before 0.5 factor (round up to integer)
      TS_ASSERT_EQUALS(d6, 2);
      TS_ASSERT_EQUALS(d7, 1);
    }

    std::string outfile3 = "./LAUE3";
    SaveLauenorm alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize())
    TS_ASSERT(alg3.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg3.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("Filename", outfile3));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("EliminateBankNumbers", "1"));
    TS_ASSERT_THROWS_NOTHING(alg3.execute(););
    TS_ASSERT(alg3.isExecuted());
    // Get the file
    outfile3 = alg3.getPropertyValue("Filename") + "001";
    // file does not exist because all peaks are bank1 which were eliminated
    TS_ASSERT(!Poco::File(outfile3).exists());
    remove(outfile.c_str());
    remove(outfile2.c_str());
    remove(outfile3.c_str());

    // Now try with lscale format
    std::string outfile4 = "./LSCALE";
    LoadIsawPeaks loadPeaks;
    loadPeaks.initialize();
    loadPeaks.setPropertyValue("FileName", "Peaks5637.integrate");
    loadPeaks.setPropertyValue("OutputWorkspace", "abc");
    loadPeaks.execute();
    PeaksWorkspace_sptr ows =
        std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("abc"));
    SaveLauenorm alg4;
    TS_ASSERT_THROWS_NOTHING(alg4.initialize())
    TS_ASSERT(alg4.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg4.setProperty("InputWorkspace", ows));
    TS_ASSERT_THROWS_NOTHING(alg4.setPropertyValue("Filename", outfile4));
    TS_ASSERT_THROWS_NOTHING(alg4.setProperty("LaueScaleFormat", true));
    TS_ASSERT_THROWS_NOTHING(alg4.setProperty("CrystalSystem", "RHOMBOHEDRAL"));
    TS_ASSERT_THROWS_NOTHING(alg4.setProperty("Centering", "R"));
    TS_ASSERT_THROWS_NOTHING(alg4.execute(););
    TS_ASSERT(alg4.isExecuted());
    // Get the file
    outfile4 = alg4.getPropertyValue("Filename") + "001.geasc";
    TS_ASSERT(Poco::File(outfile4).exists());

    std::ifstream in4(outfile4.c_str());
    std::string line;
    if (numPeaksPerBank > 0) {
      for (int i = 0; i < 7; i++)
        getline(in4, line);
      in4 >> line >> d1 >> d2 >> d3 >> d4;
      TS_ASSERT_EQUALS(d1, 6);
      TS_ASSERT_EQUALS(d2, 7);
      TS_ASSERT_EQUALS(d3, 1);
      TS_ASSERT_EQUALS(d4, 3);
    }
    // remove(outfile4.c_str());
  }

  /// Test with a few peaks
  void test_exec() { do_test(2, 4, 4); }
};
