// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ISISDATAARCHIVETEST_H_
#define ISISDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidDataHandling/ISISDataArchive.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {
#ifdef _WIN32
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/windir?name=";
#else
const char *URL_PREFIX = "http://data.isis.rl.ac.uk/where.py/unixdir?name=";
#endif
}

class MockgetCorrectExtension : public ISISDataArchive {
public:
  std::string
  getCorrectExtension(const std::string &path,
                      const std::vector<std::string> &exts) const override {
    (void)exts;
    return path;
  }
};

class ISISDataArchiveTest : public CxxTest::TestSuite {
public:
  /**
   * Un-'x' this test name to test locally.
   * This tests the filename loop part of `getArchivePath`
   * i.e. for each of the filenames in the set, it makes a call to the archive
   * and `getCorrectExtension` is mocked out.
   */
  void xtestFilenameLoop() {
    MockgetCorrectExtension arch;

    std::set<std::string> filename;
    filename.insert("hrpd273");
    const std::vector<std::string> extension = std::vector<std::string>(1, "");
    const std::string path = arch.getArchivePath(filename, extension);
    if (path.empty()) {
      TS_FAIL("Returned path was empty.");
    } else {
      TS_ASSERT_EQUALS(path.substr(path.size() - 18, 10), "cycle_98_0");
    }
  }

  /**
   * Un-'x' this test name to test locally.
   * This tests the file extensions loop.
   * To run, this tests requires that the ISIS archive is on your local machine.
   */
  void xtestgetCorrectExtension() {
    std::string path;
    if (strcmp(URL_PREFIX, "http://data.isis.rl.ac.uk/where.py/windir?name=") ==
        0) {
      path = "\\isis.cclrc.ac.uk\\inst$\\ndxhrpd\\instrument\\data\\cycle_98_"
             "0\\HRP00273";
    } else {
      path = "/archive/ndxhrpd/Instrument/data/cycle_98_0/HRP00273";
    }
    ISISDataArchive arch;

    const std::vector<std::string> correct_exts = {".RAW"};
    const std::string rawExtension =
        arch.getCorrectExtension(path, correct_exts);
    TS_ASSERT_EQUALS(rawExtension, path + ".RAW");

    const std::vector<std::string> incorrect_exts = {".so", ".txt"};
    const std::string emptyString =
        arch.getCorrectExtension(path, incorrect_exts);
    TS_ASSERT_EQUALS(emptyString, "");
  }

  void testFactory() {
    boost::shared_ptr<IArchiveSearch> arch =
        ArchiveSearchFactory::Instance().create("ISISDataSearch");
    TS_ASSERT(arch);
  }
};

#endif /*ISISDATAARCHIVETEST_H_*/
