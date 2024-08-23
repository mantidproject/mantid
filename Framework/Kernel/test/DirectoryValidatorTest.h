// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DirectoryValidator.h"
#include <Poco/File.h>
#include <filesystem>
#include <fstream>

using namespace Mantid::Kernel;

class DirectoryValidatorTest : public CxxTest::TestSuite {
public:
  void testFailsOnNonexistentDirectory() {
    DirectoryValidator v(true);
    std::string NoDir("/home/MyJunkyFolderThatDoesntExist");
    TS_ASSERT_EQUALS(v.isValid(NoDir), "Directory \"" + NoDir + "\" not found");
  }

  void testFailsOnAFile() {
    DirectoryValidator v(true);
    std::string ThisIsAFile("directoryvalidatortestfile.txt");

    std::filesystem::path txt_file(ThisIsAFile);
    std::ofstream handle(txt_file);
    handle.close();
    TS_ASSERT_EQUALS(v.isValid(ThisIsAFile), "Directory \"" + ThisIsAFile + "\" specified is actually a file");
    std::filesystem::remove(txt_file);
  }

  void testPassesOnNonexistentDirectoryIfYouSaySoForSomeReason() {
    DirectoryValidator v(false);
    std::string NoDir("./MyJunkyFolderThatDoesntExist");
    TS_ASSERT_EQUALS(v.isValid(NoDir), "");
  }

  void testPassesOnExistingDirectory() {
    std::string TestDir("./MyTestFolder");
    std::filesystem::path dir(TestDir);
    TS_ASSERT(std::filesystem::create_directory(dir));
    DirectoryValidator v(true);
    TS_ASSERT_EQUALS(v.isValid(TestDir), "");
    std::filesystem::remove(dir); // clean up your folder
  }
};
