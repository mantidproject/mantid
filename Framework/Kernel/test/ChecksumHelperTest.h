// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ChecksumHelper.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>

using namespace Mantid::Kernel;

class ChecksumHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChecksumHelperTest *createSuite() { return new ChecksumHelperTest(); }
  static void destroySuite(ChecksumHelperTest *suite) { delete suite; }

  void testMd5FromString() {
    std::string response = ChecksumHelper::md5FromString("Test this string out for size");
    TSM_ASSERT_EQUALS("The calculated MD5 hash is not as expected", "8061266bbcc3f758359d3ecee24904e6", response);
  }

  void testSha1FromString() {
    std::string response = ChecksumHelper::sha1FromString("Test this string out for size");
    TSM_ASSERT_EQUALS("The calculated SHA-1 hash is not as expected", "c1c9af231c340826bdabd33eae076d5e532eba08",
                      response);
  }

  void testSha1FromFile() {
    const std::string filename("ChecksumHelperTest_testSha1FromFile.txt");
    const std::string data = "ChecksumHelperTest_testSha1FromFile Test this "
                             "string out for size in a file";
    createFile(filename, data);

    std::string response = ChecksumHelper::sha1FromFile(filename, false);
    TSM_ASSERT_EQUALS("The calculated SHA-1 hash is not as expected", "363cbe9c113b8bcba9e0aa94dbe45e67856ff26b",
                      response);
    std::filesystem::remove(filename);
  }

  void testGitSha1FromFile() {
    const std::string filename("ChecksumHelperTest_testGitSha1FromFile.txt");
    const std::string data = "ChecksumHelperTest_testGitSha1FromFile Test this "
                             "string out for size in a file";
    createFile(filename, data);

    std::string response = ChecksumHelper::gitSha1FromFile(filename);
    TSM_ASSERT_EQUALS("The calculated git-hash is not as expected", "db46957d5afdb266b4b3321f3ce2b8887f190ff5",
                      response);
    std::filesystem::remove(filename);
  }

  void testGitSha1FromFileWithLinuxLineEndings() {
    const std::string filename("ChecksumHelperTest_testGitSha1FromFileWithLinuxLineEndings.txt");
    const std::string data = "ChecksumHelperTest_"
                             "testGitSha1FromFileWithLinuxLineEndings\nTest "
                             "this string out for size\n in a file";
    createFile(filename, data);

    std::string response = ChecksumHelper::gitSha1FromFile(filename);
    TSM_ASSERT_EQUALS("The calculated git-hash is not as expected", "7e78655a4e48aa2fbd4a3f1aec4043009e342e31",
                      response);
    std::filesystem::remove(filename);
  }

  void testGitSha1FromFileWithWindowsLineEndingsFirstConvertsToLF() {
    const std::string filename("ChecksumHelperTest_testGitSha1FromFileWithWindowsLineEndings.txt");
    const std::string data = "ChecksumHelperTest_"
                             "testGitSha1FromFileWithWindowsLineEndings\r\nTest"
                             " this string out for size\r\n in a file";
    createFile(filename, data);

    std::string response = ChecksumHelper::gitSha1FromFile(filename);
    TSM_ASSERT_EQUALS("The calculated git-hash is not as expected", "23dcaeaefce51ed7cae98f6420f67e0ba0e2058a",
                      response);
    std::filesystem::remove(filename);
  }

  void testGitSha1FromFileWithOldStyleMacLineEndingsDoesNotConvertToLF() {
    const std::string filename("ChecksumHelperTest_testGitSha1FromFileWithOldStyleMacLineEndings.txt");
    const std::string data = "ChecksumHelperTest_"
                             "testGitSha1FromFileWithLinuxLineEndings\rTest "
                             "this string out for size\r in a file";
    createFile(filename, data);

    std::string response = ChecksumHelper::gitSha1FromFile(filename);
    TSM_ASSERT_EQUALS("The calculated git-hash is not as expected", "7b7e77332c1610df14fd26476d1601a22a34f11f",
                      response);
    std::filesystem::remove(filename);
  }

  void createFile(const std::string &fileName, const std::string &data) {
    // has to be saved as binary so it doesn't mess with line endings
    std::ofstream file(fileName.c_str(), std::ofstream::binary);
    file << data;
    file.close();
  }
};
