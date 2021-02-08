// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FileValidator.h"
#include <Poco/File.h>

#if defined(__GNUC__) || defined(__clang__)
#include "boost/filesystem.hpp"
#endif

using namespace Mantid::Kernel;

class FileValidatorTest : public CxxTest::TestSuite {
public:
  void testConstructors() {
    // empty constructor
    FileValidator v1;
    TS_ASSERT_EQUALS(v1.allowedValues().size(), 0);

    // one with a vector of extensions
    FileValidator v2({"raw", "RAW"});

    // File extensions are converted to lowercase so should have one unique
    // extension
    TS_ASSERT_EQUALS(v2.allowedValues().size(), 1);
  }

  void testPassesOnExistentFile() {
    // Create two files, one with the extension within the validator and one
    // without

    const std::string file_stub = "scratch.";
    const std::string ext1 = "txt";
    const std::string ext2 = "raw";
    Poco::File txt_file(file_stub + ext1);
    Poco::File raw_file(file_stub + ext2);

    try {
      txt_file.createFile();
      raw_file.createFile();
    } catch (std::exception &) {
      TS_FAIL(
          "Error creating test file for \"testPassesOnExistentFile\" test.");
    }

    // FileValidator will suggest txt files as correct extension
    std::vector<std::string> vec(1, "txt");
    FileValidator v1(vec);

    TS_ASSERT_EQUALS(v1.isValid(txt_file.path()), "");
    // Not correct extension but the file exists so we allow it
    TS_ASSERT_EQUALS(v1.isValid(raw_file.path()), "");

    txt_file.remove();
    raw_file.remove();
  }

  void testPassesForMoreComplicatedExtensions() {
    // More general test cases (Refs #1302)
    const std::string file_stub = "scratch";
    const std::string ext1 = ".tar.gz";
    const std::string ext2 = "_event.dat";
    Poco::File txt_file(file_stub + ext1);
    Poco::File raw_file(file_stub + ext2);
    try {
      txt_file.createFile();
      raw_file.createFile();
    } catch (std::exception &) {
      TS_FAIL("Error creating test file for "
              "\"testPassesForMoreComplicatedExtensions\" test.");
    }

    // FileValidator will suggest txt files as correct extension
    std::vector<std::string> vec(1, ".tar.gz");
    FileValidator v1(vec);

    TS_ASSERT_EQUALS(v1.isValid(txt_file.path()), "");
    // Not correct extension but the file exists so we allow it
    TS_ASSERT_EQUALS(v1.isValid(raw_file.path()), "");

    txt_file.remove();
    raw_file.remove();
  }

  void testFailsOnNonexistentFile() {
    std::string NoFile("myJunkFile_hgfvj.cpp");
    std::vector<std::string> vec{"cpp"};
    FileValidator v(vec);
    TS_ASSERT_EQUALS(v.isValid(NoFile), "File \"" + NoFile + "\" not found");
  }

  void testPassesOnNonexistentFile() {
    std::string NoFile("myJunkFile_hgfvj.cpp");
    std::vector<std::string> vec{"cpp"};
    FileValidator v(vec, false);
    TS_ASSERT_EQUALS(v.isValid(NoFile), "");
  }
  void testFailsIfNoPermissions() {
#if defined(__GNUC__) || defined(__clang__)
    const char *filename = "testfile.txt";
    Poco::File txt_file(filename);
    txt_file.createFile();
    boost::filesystem::permissions(filename,
                                   boost::filesystem::perms::owner_read |
                                       boost::filesystem::remove_perms);
    std::vector<std::string> vec;
    FileValidator v(vec);

    TS_ASSERT_EQUALS(v.isValid(txt_file.path()),
                     "Failed to open testfile.txt: Permission denied");
    txt_file.remove();
#endif
  }

  void testFailsOnEmptyFileString() {
    FileValidator file_val;
    TS_ASSERT_EQUALS(file_val.isValid(""), "File \"\" not found");
  }
};
