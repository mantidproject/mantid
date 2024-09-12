// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FileValidator.h"
#include <filesystem>
#include <fstream>

using namespace Mantid::Kernel;

namespace {
void createEmpty(const std::filesystem::path &path) {
  std::ofstream handle(path);
  handle.close();
}
} // namespace

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
    std::filesystem::path txt_file(file_stub + ext1);
    std::filesystem::path raw_file(file_stub + ext2);

    try {
      createEmpty(txt_file);
      createEmpty(raw_file);
    } catch (std::exception &) {
      TS_FAIL("Error creating test file for \"testPassesOnExistentFile\" test.");
    }

    // FileValidator will suggest txt files as correct extension
    std::vector<std::string> vec(1, "txt");
    FileValidator v1(vec);

    TS_ASSERT_EQUALS(v1.isValid(txt_file.string()), "");
    // Not correct extension but the file exists so we allow it
    TS_ASSERT_EQUALS(v1.isValid(raw_file.string()), "");

    std::filesystem::remove(txt_file);
    std::filesystem::remove(raw_file);
  }

  void testPassesForMoreComplicatedExtensions() {
    // More general test cases (Refs #1302)
    const std::string file_stub = "scratch";
    const std::string ext1 = ".tar.gz";
    const std::string ext2 = "_event.dat";
    std::filesystem::path txt_file(file_stub + ext1);
    std::filesystem::path raw_file(file_stub + ext2);
    try {
      createEmpty(txt_file);
      createEmpty(raw_file);
    } catch (std::exception &) {
      TS_FAIL("Error creating test file for "
              "\"testPassesForMoreComplicatedExtensions\" test.");
    }

    // FileValidator will suggest txt files as correct extension
    std::vector<std::string> vec(1, ".tar.gz");
    FileValidator v1(vec);

    TS_ASSERT_EQUALS(v1.isValid(txt_file.string()), "");
    // Not correct extension but the file exists so we allow it
    TS_ASSERT_EQUALS(v1.isValid(raw_file.string()), "");

    std::filesystem::remove(txt_file);
    std::filesystem::remove(raw_file);
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
    std::filesystem::path txt_file(filename);
    createEmpty(txt_file);
    std::filesystem::permissions(filename, std::filesystem::perms::owner_read, std::filesystem::perm_options::remove);
    std::vector<std::string> vec;
    FileValidator v(vec);

    TS_ASSERT_EQUALS(v.isValid(txt_file.string()), "Failed to open testfile.txt: Permission denied");
    std::filesystem::remove(txt_file);
#endif
  }

  void testFailsOnEmptyFileString() {
    FileValidator file_val;
    TS_ASSERT_EQUALS(file_val.isValid(""), "File \"\" not found");
  }
};
