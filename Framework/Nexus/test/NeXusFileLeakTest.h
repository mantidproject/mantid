// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

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
using namespace NexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class NeXusFileLeakTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NeXusFileLeakTest *createSuite() { return new NeXusFileLeakTest(); }
  static void destroySuite(NeXusFileLeakTest *suite) { delete suite; }

  /**
   * These correspond to former napi tests
   * - leak_test1
   * - leak_test2
   * - leak_test3
   */

  void test_leak1() {
    int const nReOpen = 1000;
    cout << "Running Leak Test 1: " << nReOpen << " iterations\n";
    string const szFile("nexus_leak_test1_h4.nxs");

    File file_obj(szFile, NXACC_CREATE4);
    std::string oss(strmakef("entry_%d", 0));
    file_obj.makeGroup(oss, "NXentry");
    file_obj.openGroup(oss, "NXentry");
    std::string oss2(strmakef("data_%d", 0));
    file_obj.makeGroup(oss2, "NXdata");
    file_obj.openGroup(oss2, "NXdata");
    std::string oss3(strmakef("data_%d", 0));
    file_obj.makeGroup(oss3, "NXdata");
    file_obj.openGroup(oss3, "NXdata");

    file_obj.close();

    // for (int iReOpen = 0; iReOpen < nReOpen; iReOpen++) {
    //   if (0 == iReOpen % 100) {
    //     cout << "loop count " << iReOpen << "\n";
    //   }

    //  file_obj = File(szFile, NXACC_RDWR);
    //  file_obj.close();
    //}

    cout << "Leak Test 1 Success!\n";
  }

  void test_leak2() {
    int const nFiles = 10;
    int const nEntry = 10;
    int const nData = 10;
    vector<int16_t> const i2_array{1000, 2000, 3000, 4000};

    cout << "Running Leak Test 2: " << nFiles << " iterations\n";

    NXaccess access_mode = NXACC_CREATE4;
    std::string strFile;

    for (int iFile = 0; iFile < nFiles; iFile++) {
      strFile = strmakef("nexus_leak_test2_%03d_h4.nxs", iFile);
      removeFile(strFile);
      cout << "file " << strFile << "\n";

      File fileid(strFile, access_mode);

      for (int iEntry = 0; iEntry < nEntry; iEntry++) {
        std::string oss(strmakef("entry_%d", iEntry));
        fileid.makeGroup(oss, "NXentry");
        fileid.openGroup(oss, "NXentry");
        for (int iNXdata = 0; iNXdata < nData; iNXdata++) {
          std::string oss2(strmakef("data_%d", iNXdata));
          fileid.makeGroup(oss2, "NXdata");
          fileid.openGroup(oss2, "NXdata");
          for (int iData = 0; iData < nData; iData++) {
            std::string oss3(strmakef("i2_data_%d", iData));
            std::vector<int64_t> dims({(int64_t)i2_array.size()});
            fileid.makeData(oss3, NXnumtype::INT16, dims);
            fileid.openData(oss3);
            fileid.putData(i2_array.data());
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
