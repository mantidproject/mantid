// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include <cxxtest/TestSuite.h>

#include <filesystem>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FileLoaderRegistryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor (& destructor) isn't called when running other tests
  static FileLoaderRegistryTest *createSuite() { return new FileLoaderRegistryTest(); }
  static void destroySuite(FileLoaderRegistryTest *suite) { delete suite; }

  FileLoaderRegistryTest() {}

  ~FileLoaderRegistryTest() override {}

  void runCheck(const std::string &filename, const std::string &alg_expected, const int version_expected) {
    std::string path = FileFinder::Instance().getFullPath(filename);

    TS_ASSERT(!path.empty());

    // algorithm can load the file
    // TS_ASSERT(FileLoaderRegistry::Instance().canLoad(alg_expected, path));

    // get the right algorithm
    try {
      const auto loader = FileLoaderRegistry::Instance().chooseLoader(path);
      if (loader) {
        TSM_ASSERT_EQUALS(filename, loader->name(), alg_expected);
        TSM_ASSERT_EQUALS(filename, loader->version(), version_expected);
      } else {
        TS_FAIL("Failed to find a loader for " + filename);
      }
    } catch (const std::runtime_error &e) {
      TS_FAIL(e.what()); // no exception should be thrown
    }
  }

  void testMuon() {
    runCheck("MUSR00022725.nxs", "LoadMuonNexus", 1);
    runCheck("emu00006473.nxs", "LoadMuonNexus", 1);
  }

  void testSNS() {
    runCheck("TOPAZ_3007.peaks.nxs", "LoadNexusProcessed", 2);
    runCheck("PG3_733.nxs", "LoadNexusProcessed", 2);

    runCheck("REF_L_183110.nxs.h5", "LoadEventNexus", 1);
    runCheck("CNCS_7860_event.nxs", "LoadEventNexus", 1);
  }

private:
};
