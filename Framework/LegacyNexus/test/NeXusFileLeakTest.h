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
   */

  void test_leak1() {
    int const nReOpen = 1000;
    cout << "Running Leak Test 1: " << nReOpen << " iterations\n";
    const std::string szFile = Mantid::API::FileFinder::Instance().getFullPath("LegacyNexus/hdf5/nexus_leak_test1.nxs");

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

  void test_leak2() {
    int const nFiles = 1;
    int const nEntry = 10;
    int const nData = 10;
    vector<int16_t> const i2_array{1000, 2000, 3000, 4000};

    cout << "Running Leak Test 2: " << nFiles << " iterations\n";

    NXaccess access_mode = NXACC_READ;
    std::string strFile;

    for (int iFile = 0; iFile < nFiles; iFile++) {
      strFile = strmakef("LegacyNexus/hdf5/nexus_leak_test2_%03d.nxs", iFile);
      const std::string szFile = Mantid::API::FileFinder::Instance().getFullPath(strFile);
      cout << "file " << szFile << "\n";

      File fileid(szFile, access_mode);

      for (int iEntry = 0; iEntry < nEntry; iEntry++) {
        std::string oss(strmakef("entry_%d", iEntry));
        fileid.openGroup(oss, "NXentry");
        for (int iNXdata = 0; iNXdata < nData; iNXdata++) {
          std::string oss2(strmakef("data_%d", iNXdata));
          fileid.openGroup(oss2, "NXdata");
          for (int iData = 0; iData < nData; iData++) {
            std::string oss3(strmakef("i2_data_%d", iData));
            std::vector<int64_t> dims({(int64_t)i2_array.size()});
            fileid.openData(oss3);
            fileid.closeData();
          }
          fileid.closeGroup();
        }
        fileid.closeGroup();
      }
      fileid.close();
    }
    cout << "Leak Test 2 Success!\n";
  }
};
