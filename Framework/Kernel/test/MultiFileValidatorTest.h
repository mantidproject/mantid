// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MultiFileValidator.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
// Convenience function, that wraps a string in a vector and adds it to a vector
// of vectors.
void addSingleFile(std::vector<std::vector<std::string>> &fileNames, const std::string &fileNameToAdd) {
  const std::vector<std::string> fileNameList(1, fileNameToAdd);
  fileNames.emplace_back(fileNameList);
}

void createEmpty(const std::filesystem::path &path) {
  std::ofstream handle(path);
  handle.close();
}
} // namespace

using namespace Mantid::Kernel;

class MultiFileValidatorTest : public CxxTest::TestSuite {
public:
  void testVectorConstructor() {
    std::vector<std::string> vec{"raw", "RAW"};
    FileValidator v(vec);

    // File extensions are converted to lowercase so should have one unique
    // extension
    TS_ASSERT_EQUALS(v.allowedValues().size(), 1);
  }

  void testCopyConstructor() {
    FileValidator v({"raw", "RAW"});
    FileValidator copy(v);

    // File extensions are converted to lowercase so should have one unique
    // extension
    TS_ASSERT_EQUALS(copy.allowedValues().size(), 1);
  }

  void testPassesOnExistentFiles() {
    // Create two pairs of files, one with the extension within the validator
    // and one without.
    const std::string file_stub = "scratch.";
    const std::string ext1 = "txt";
    const std::string ext2 = "raw";
    std::filesystem::path txt_file_1(file_stub + "_1" + ext1);
    std::filesystem::path txt_file_2(file_stub + "_2" + ext1);
    std::filesystem::path raw_file_1(file_stub + "_1" + ext2);
    std::filesystem::path raw_file_2(file_stub + "_2" + ext2);

    std::vector<std::vector<std::string>> txt_files;
    std::vector<std::vector<std::string>> raw_files;

    try {
      createEmpty(txt_file_1);
      createEmpty(txt_file_2);

      createEmpty(raw_file_1);
      createEmpty(raw_file_2);
    } catch (std::exception &) {
      TS_FAIL("Error creating test file for \"testPassesOnExistentFile\" test.");
    }

    addSingleFile(txt_files, txt_file_1.string());
    addSingleFile(txt_files, txt_file_2.string());
    addSingleFile(raw_files, txt_file_1.string());
    addSingleFile(raw_files, txt_file_2.string());

    // FileValidator will suggest txt files as correct extension
    std::vector<std::string> vec(1, "txt");
    MultiFileValidator v1(vec);

    TS_ASSERT_EQUALS(v1.isValid(txt_files), "");
    // Not correct extension but the file exists so we allow it
    TS_ASSERT_EQUALS(v1.isValid(raw_files), "");

    std::filesystem::remove(txt_file_1);
    std::filesystem::remove(txt_file_2);
    std::filesystem::remove(raw_file_1);
    std::filesystem::remove(raw_file_2);
  }

  void testFailsOnSomeNonExistentFiles() {
    // Create two files, numbered 1 and 3.
    const std::string file_stub = "scratch.";
    const std::string ext = "txt";
    std::filesystem::path txt_file_1(file_stub + "_1" + ext);
    std::filesystem::path txt_file_3(file_stub + "_3" + ext);

    std::vector<std::vector<std::string>> txt_files;

    try {
      createEmpty(txt_file_1);
      createEmpty(txt_file_3);
    } catch (std::exception &) {
      TS_FAIL("Error creating test file for \"testPassesOnExistentFile\" test.");
    }

    addSingleFile(txt_files, txt_file_1.string());
    addSingleFile(txt_files, "doesNotExist_2.txt");
    addSingleFile(txt_files, txt_file_3.string());
    addSingleFile(txt_files, "doesNotExist_4.txt");

    // FileValidator will suggest txt files as correct extension
    std::vector<std::string> vec(1, "txt");
    MultiFileValidator v(vec);

    TS_ASSERT_EQUALS(v.isValid(txt_files), "Could not validate the following "
                                           "file(s): doesNotExist_2.txt, "
                                           "doesNotExist_4.txt");

    // Clean up.
    std::filesystem::remove(txt_file_1);
    std::filesystem::remove(txt_file_3);
  }

  void testFailsOnNoFiles() {
    MultiFileValidator file_val;
    TS_ASSERT_EQUALS(file_val.isValid(std::vector<std::vector<std::string>>()).empty(), false);
  }

  void testFailsOnNonExistingFiles() {
    std::vector<std::string> vec{"foo"};
    MultiFileValidator file_val(vec);
    std::vector<std::vector<std::string>> file{{"myJunkFile.foo"}};
    TS_ASSERT(!file_val.isValid(file).empty());
  }

  void testPassesOnNonExistingFiles() {
    std::vector<std::string> vec{"foo"};
    MultiFileValidator file_val(vec, false);
    std::vector<std::vector<std::string>> file{{"myJunkFile.foo"}};
    TS_ASSERT(file_val.isValid(file).empty());
  }
};
