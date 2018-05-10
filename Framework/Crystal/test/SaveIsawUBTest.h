/* SaveIsawUBTest.h
 *
 *  Created on: Aug 12, 2011
 *      Author: ruth
 */

#ifndef MANTID_CRYSTAL_SAVEISAWUBTEST_H_
#define MANTID_CRYSTAL_SAVEISAWUBTEST_H_

#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/SaveIsawUB.h"

#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("LoadIsawUBTest_ws", ws);

    std::string File1, File2;

    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", "TOPAZ_3007.mat"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_ws"));
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
    TS_ASSERT_THROWS_NOTHING(
        Salg.setProperty("Filename", "TOPAZ_3007_resaved.mat"));
    TS_ASSERT_THROWS_NOTHING(
        Salg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_ws"));
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

    int line = 1;
    std::string s;
    double val1, val2;
    double tolerance;

    if (F1.good() && F2.good())
      for (int row = 0; row < 5; row++) {
        int NNums = 3;
        tolerance = .0000003;
        if (line > 3) {
          NNums = 7;
          tolerance = .0003;
        }

        for (int N = 0; N < NNums; N++) {
          s = Mantid::Kernel::Strings::getWord(F1, false);

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
    else {
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
};

#endif /* SAVEISAWUBTEST_H_ */
