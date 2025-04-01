// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
#include "MantidLegacyNexus/NeXusFile.hpp"
#include "test_helper.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace Mantid::LegacyNexus;
using namespace LegacyNexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class LegacyNeXusFileLeakTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LegacyNeXusFileLeakTest *createSuite() { return new LegacyNeXusFileLeakTest(); }
  static void destroySuite(LegacyNeXusFileLeakTest *suite) { delete suite; }

  /**
   * These correspond to former napi tests
   * - leak_test1
   * - leak_test2
   * - leak_test3
   */

  void test_leak1() {
    int const nReOpen = 1000;
    cout << "Running Leak Test 1: " << nReOpen << " iterations\n";
    const std::string szFile = Mantid::API::FileFinder::Instance().getFullPath("nexus_leak_test1.nxs");

    for (int iReOpen = 0; iReOpen < nReOpen; iReOpen++) {
      if (0 == iReOpen % 100) {
        cout << "loop count " << iReOpen << "\n";
      }

      File file_obj(szFile, NXACC_READ);
      file_obj.openGroup("entry_0", "NXentry");
      // file_obj.openData("data_0");
      // file_obj.closeData(); ### Accidently created the data as a group, redo this file. Loop to create lots of
      // groups/data like test 2.
      file_obj.closeGroup();
      file_obj.close();

      // fileid.openGroup(oss, "NXentry"); Lets open and close a few groups?
    }
    cout << "Leak Test 1 Success!\n";
  }
};
