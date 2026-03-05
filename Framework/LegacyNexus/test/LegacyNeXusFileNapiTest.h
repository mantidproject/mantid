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
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

int setEnvVarFromString(std::string &envVarStr) {
#ifdef _WIN32
  return _putenv(envVarStr.c_str());
#else
  auto pos = envVarStr.find('=');
  std::string envVar = envVarStr.substr(0, pos);
  std::string value = envVarStr.substr(pos + 1);
  if (value != "") {
    return setenv(envVar.c_str(), value.c_str(), 1);
  } else {
    return unsetenv(envVar.c_str());
  }

#endif
}

using namespace Mantid::LegacyNexus;
using namespace LegacyNexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class LegacyNeXusFileNapiTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LegacyNeXusFileNapiTest *createSuite() { return new LegacyNeXusFileNapiTest(); }
  static void destroySuite(LegacyNeXusFileNapiTest *suite) { delete suite; }

private:
  void do_test_read(const string &filename) {
    cout << "readTest(" << filename << ") started\n";
    const string SDS("SDS");
    // top level file information
    File file(filename);
    file.openGroup("entry", "NXentry");

    // Test getDataCoerce() -------------------
    std::vector<int> ints;
    std::vector<double> doubles;

    ints.clear();
    file.openData("i1_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1);
    file.closeData();

    ints.clear();
    file.openData("i2_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1000);
    file.closeData();

    ints.clear();
    file.openData("i4_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1000000)
    file.closeData();

    doubles.clear();
    file.openData("r4_data");
    file.getDataCoerce(doubles);
    TS_ASSERT_EQUALS(doubles.size(), 20);
    TS_ASSERT_EQUALS(doubles[1], 1.0)
    file.closeData();

    doubles.clear();
    file.openData("r8_data");
    file.getDataCoerce(doubles);
    TS_ASSERT_EQUALS(doubles.size(), 20);
    TS_ASSERT_EQUALS(doubles[1], 21.0)
    file.closeData();

    // Throws when you coerce to int from a real/double source
    ints.clear();
    file.openData("r8_data");
    TS_ASSERT_THROWS_ANYTHING(file.getDataCoerce(ints));
    file.closeData();

    // Close the "entry" group
    file.closeGroup();

    // openpath checks
    file.openPath("/entry/data/comp_data");
    file.openPath("/entry/data/comp_data");
    file.openPath("../r8_data");
    cout << "NXopenpath checks OK\n";

    // everything went fine
    cout << "readTest(" << filename << ") successful\n";
  }

  void do_test_loadPath(const string &filename) {
    TS_ASSERT_THROWS_NOTHING(File file(filename, NXACC_READ));
    cout << "Success loading NeXus file from path" << endl;
  }

public:
  void test_read_h5() { impl_test_read(NexusFormat::HDF5); }
  void test_read_h4() { impl_test_read(NexusFormat::HDF4); }

  void impl_test_read(NexusFormat fmt) {
    cout << " Nexus File Tests\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "nexus_file_napi_test_cpp");
    const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    // try reading a file
    do_test_read(filepath);

    int envSet = 1; // non 0 value indicates path env var not set
    // try using the load path
    std::string blankVarStr = "NX_LOAD_PATH=";
    if (getenv("NX_LOAD_PATH") == NULL) {
      std::string envStr = blankVarStr + filepath;
      envStr.erase(envStr.find(vars.relFilePath));
      envSet = setEnvVarFromString(envStr);
    }
    do_test_loadPath(vars.relFilePath);

    // clean load path
    if (envSet == 0) {
      (void)setEnvVarFromString(blankVarStr);
    }
  }
};
