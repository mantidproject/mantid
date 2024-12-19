// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/* SaveIsawUBTest.h
 *
 *  Created on: Aug 12, 2011
 *      Author: ruth
 */

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/SaveIsawUB.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using namespace Mantid::Kernel::Strings;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace std;

class SaveIsawUBTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveIsawUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("LoadIsawUBTest_ws", ws);

    std::string File1, File2;

    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", "TOPAZ_3007.mat"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Get the full path to the found file
    File1 = alg.getPropertyValue("Filename");

    TS_ASSERT(ws);
    if (!ws)
      return;

    SaveIsawUB Salg;
    TS_ASSERT_THROWS_NOTHING(Salg.initialize())
    TS_ASSERT(Salg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(Salg.setProperty("Filename", "TOPAZ_3007_resaved.mat"));
    TS_ASSERT_THROWS_NOTHING(Salg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_ws"));
    TS_ASSERT_THROWS_NOTHING(Salg.execute(););
    TS_ASSERT(Salg.isExecuted());
    // Check the results

    // Get the full path to the saved file
    File2 = Salg.getPropertyValue("Filename");

    AnalysisDataService::Instance().remove("LoadIsawUBTest_ws");

    ifstream F1;
    ifstream F2;
    F1.open(File1.c_str());
    F2.open(File2.c_str());

    std::string s;

    if (F1.good() && F2.good()) {
      int line = 1;
      for (int row = 0; row < 5; row++) {
        int NNums = 3;
        double tolerance = .0000003;
        if (line > 3) {
          NNums = 7;
          tolerance = .0003;
        }

        for (int N = 0; N < NNums; N++) {
          s = Mantid::Kernel::Strings::getWord(F1, false);

          double val1, val2;
          if (!Mantid::Kernel::Strings::convert<double>(s, val1)) {
            stringstream message;
            message << "Characters on line " << line << " word " << N;
            message << " in the original file does not represent a number";
            TS_FAIL(message.str());
          }

          s = Mantid::Kernel::Strings::getWord(F2, false);
          if (!Mantid::Kernel::Strings::convert<double>(s, val2)) {
            stringstream message;
            message << "Characters on line " << line << " word " << N;
            message << " in the saved file does not represent a number";
            TS_FAIL(message.str());
          }
          if (line < 4)
            TS_ASSERT_DELTA(val1, val2, tolerance);
        }
        Mantid::Kernel::Strings::readToEndOfLine(F1, true);
        Mantid::Kernel::Strings::readToEndOfLine(F2, true);
        line++;
      }
    } else {
      TS_ASSERT(F1.good());
      TS_ASSERT(F2.good());
      remove(File2.c_str());
      return;
    }

    for (int row = 0; row < 6; row++) {
      std::string s1 = Mantid::Kernel::Strings::getWord(F1, false);
      std::string s2 = Mantid::Kernel::Strings::getWord(F2, false);
      while (s1.length() > 0 && s2.length() > 0) {
        TS_ASSERT_EQUALS(s1, s2);
        s1 = Mantid::Kernel::Strings::getWord(F1, false);
        s2 = Mantid::Kernel::Strings::getWord(F2, false);
      }

      TS_ASSERT_EQUALS(s1.length(), s2.length());

      Mantid::Kernel::Strings::readToEndOfLine(F1, true);
      Mantid::Kernel::Strings::readToEndOfLine(F2, true);
    }

    remove(File2.c_str());
  }

  void test_exec_rotate_by_gonio() {
    // Fake output WS
    std::string wsname = "ws_rot_gonio";
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsname, ws);

    // set goiniomter
    auto set_gon_alg = AlgorithmFactory::Instance().create("SetGoniometer", 1);
    set_gon_alg->initialize();
    set_gon_alg->setLogging(false);
    set_gon_alg->setProperty("Workspace", wsname);
    set_gon_alg->setPropertyValue("Axis0", "90, 0.0,1.0,0.0, 1");
    set_gon_alg->execute();

    // set UB with default
    auto set_ub_alg = AlgorithmFactory::Instance().create("SetUB", 1);
    set_ub_alg->initialize();
    set_ub_alg->setLogging(false);
    set_ub_alg->setProperty("Workspace", wsname);
    set_ub_alg->execute();

    // save UB with gonio rotation applied
    SaveIsawUB save_ub_alg;
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.initialize())
    TS_ASSERT(save_ub_alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.setProperty("Filename", "SaveISawUBTest_RotGonio.mat"));
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.setProperty("RotateByGoniometerMatrix", true));
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.execute(););
    TS_ASSERT(save_ub_alg.isExecuted());

    // load UB (will now be different to one orignally applied)
    LoadIsawUB load_ub_alg;
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.initialize())
    TS_ASSERT(load_ub_alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.setProperty("Filename", save_ub_alg.getPropertyValue("Filename")));
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.execute());
    TS_ASSERT(load_ub_alg.isExecuted());

    // check UB has changed
    auto rotated_ub = ws->sample().getOrientedLattice().getUB();
    TS_ASSERT_DELTA(1.0, rotated_ub[0][0], 1e-8); // previously 0 in original (unrotated UB)
  }

  void test_exec_rotate_by_gonio_with_peaks() {
    // create workspace
    auto ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    std::string wsname = "peaks_rot_gonio";
    AnalysisDataService::Instance().addOrReplace(wsname, ws);
    // set UB
    auto set_ub_alg = AlgorithmFactory::Instance().create("SetUB", 1);
    set_ub_alg->initialize();
    set_ub_alg->setLogging(false);
    set_ub_alg->setPropertyValue("Workspace", wsname);
    set_ub_alg->execute();
    // set goniomatrix
    DblMatrix gonioMat(3, 3, true);
    gonioMat[0][0] = -1;
    gonioMat[1][1] = -1;
    ws->getPeak(0).setGoniometerMatrix(gonioMat);

    // save UB
    SaveIsawUB save_ub_alg;
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.initialize())
    TS_ASSERT(save_ub_alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.setProperty("Filename", "SaveISawUBTest_RotGonio.mat"));
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.setProperty("RotateByGoniometerMatrix", true));
    TS_ASSERT_THROWS_NOTHING(save_ub_alg.execute(););
    TS_ASSERT(save_ub_alg.isExecuted());

    // load UB (will now be different to one orignally applied)
    LoadIsawUB load_ub_alg;
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.initialize())
    TS_ASSERT(load_ub_alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.setProperty("Filename", save_ub_alg.getPropertyValue("Filename")));
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(load_ub_alg.execute());
    TS_ASSERT(load_ub_alg.isExecuted());

    // check UB has changed
    auto rotated_ub = ws->sample().getOrientedLattice().getUB();
    TS_ASSERT_DELTA(-1.0, rotated_ub[0][1], 1e-8); // previously 1 in original (unrotated UB)
  }
};
