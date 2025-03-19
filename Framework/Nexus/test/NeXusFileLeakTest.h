// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/NeXusFile.hpp"
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

using namespace NeXus;
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
    string const szFile("nexus_leak_test1.nxs");

    removeFile(szFile); // in case it was left over from previous run

    File file_obj(szFile, NXACC_CREATE5);
    file_obj.close();

    for (int iReOpen = 0; iReOpen < nReOpen; iReOpen++) {
      if (0 == iReOpen % 100) {
        cout << "loop count " << iReOpen << "\n";
      }

      file_obj = File(szFile, NXACC_RDWR);
      file_obj.close();
    }

    removeFile(szFile); // cleanup
    cout << "Leak Test 1 Success!\n";
  }

  void test_leak2() {
    int const nFiles = 10;
    int const nEntry = 10;
    int const nData = 10;
    vector<short int> const i2_array{1000, 2000, 3000, 4000};

    cout << "Running Leak Test 2: " << nFiles << " iterations\n";

    NXaccess access_mode = NXACC_CREATE5;
    std::string strFile;

    for (int iFile = 0; iFile < nFiles; iFile++) {
      strFile = strmakef("nexus_leak_test2_%03d.nxs", iFile);
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
            DimVector dims({(int64_t)i2_array.size()});
            fileid.makeData(oss3, NXnumtype::INT16, dims);
            fileid.openData(oss3);
            fileid.putData(&i2_array);
            fileid.closeData();
          }
          fileid.closeGroup();
        }
        fileid.closeGroup();
      }
      fileid.close();
      removeFile(strFile);
    }
    cout << "Leak Test 2 Success!\n";
  }

  void test_leak3() {
    cout << "Running Leak Test 3\n";
    fflush(stdout);
    const int nFiles = 10;
    const int nEntry = 2;
    const int nData = 2;
#ifdef WIN32
    // NOTE the Windows runners do not have enough stack space for the full test (max 1MB stack)
    // Rather than skip the entire test, we can use a smaller array size
    // It is no longer testing the same behavior on Windows with this choice.
    std::size_t const TEST_SIZE(8);
#else
    std::size_t const TEST_SIZE(512);
#endif // WIN32
    DimVector array_dims({TEST_SIZE, TEST_SIZE});
    std::string const szFile("nexus_leak_test3.nxs");
    int const iBinarySize = TEST_SIZE * TEST_SIZE;
    cout << "Creating array of " << iBinarySize << " integers\n";
    fflush(stdout);
    int aiBinaryData[iBinarySize];

    for (int i = 0; i < iBinarySize; i++) {
      aiBinaryData[i] = rand();
    }
    cout << "Created " << iBinarySize << " random integers\n";

    for (int iFile = 0; iFile < nFiles; iFile++) {
      cout << "file " << iFile << "\n";

      File fileid(szFile, NXACC_CREATE5);

      for (int iEntry = 0; iEntry < nEntry; iEntry++) {
        std::string oss(strmakef("entry_%d", iEntry));

        fileid.makeGroup(oss, "NXentry");
        fileid.openGroup(oss, "NXentry");
        for (int iNXdata = 0; iNXdata < nData; iNXdata++) {
          std::string oss2(strmakef("data_%d", iNXdata));
          fileid.makeGroup(oss2, "NXdata");
          fileid.openGroup(oss2, "NXdata");
          fileid.getGroupID();
          for (int iData = 0; iData < nData; iData++) {
            std::string oss3(strmakef("i2_data_%d", iData));
            fileid.makeCompData(oss3, NXnumtype::INT16, array_dims, NXcompression::LZW, array_dims);
            fileid.openData(oss3);
            fileid.putData(&aiBinaryData);
            fileid.closeData();
          }
          fileid.closeGroup();
        }
        fileid.closeGroup();
      }

      fileid.close();

      // Delete file
      removeFile(szFile);
    }
    cout << "Leak Test 3 Success!\n";
  }
};
