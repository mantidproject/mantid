// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstdio>
#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FileDescriptor.h"

#include <Poco/Path.h>
#include <filesystem>

using Mantid::Kernel::FileDescriptor;

class FileDescriptorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileDescriptorTest *createSuite() { return new FileDescriptorTest(); }
  static void destroySuite(FileDescriptorTest *suite) { delete suite; }

  FileDescriptorTest() {
    auto &cfg = Mantid::Kernel::ConfigService::Instance();
    cfg.reset();
    const auto &dataPaths = cfg.getDataSearchDirs();
    for (const auto &dataPath : dataPaths) {
      Poco::Path nxsPath(dataPath, "CNCS_7860_event.nxs");
      if (std::filesystem::exists(nxsPath.toString()))
        m_testNexusPath = nxsPath.toString();
      Poco::Path nonNxsPath(dataPath, "CSP79590.raw");
      if (std::filesystem::exists(nonNxsPath.toString()))
        m_testNonNexusPath = nonNxsPath.toString();
      Poco::Path asciiPath(dataPath, "AsciiExample.txt");
      if (std::filesystem::exists(asciiPath.toString()))
        m_testAsciiPath = asciiPath.toString();
      Poco::Path emptyFilePath(dataPath, "emptyFile.txt");
      if (std::filesystem::exists(emptyFilePath.toString()))
        m_emptyFilePath = emptyFilePath.toString();

      if (!m_testNexusPath.empty() && !m_testNonNexusPath.empty() && !m_testAsciiPath.empty() &&
          !m_emptyFilePath.empty())
        break;
    }
    if (m_testNexusPath.empty() || m_testNonNexusPath.empty() || m_testAsciiPath.empty() || m_emptyFilePath.empty()) {
      throw std::runtime_error("Unable to find test files for FileDescriptorTest. "
                               "The AutoTestData directory needs to be in the search path");
    }
  }

  //===================== Success cases
  //============================================
  void test_isAscii_Returns_True_For_Ascii_Filename() {
    TS_ASSERT(FileDescriptor::isAscii(m_testAsciiPath)); // static method

    FileDescriptor descr(m_testAsciiPath);
    TS_ASSERT(descr.isAscii());
  }

  void test_isAscii_Returns_False_For_Binary_Filename() {
    TS_ASSERT(!FileDescriptor::isAscii(m_testNonNexusPath)); // static method

    FileDescriptor descr(m_testNonNexusPath);
    TS_ASSERT(!descr.isAscii());
  }

  void test_isAscii_Returns_True_For_Stream_Pointing_At_Ascii_File_And_Stream_Is_Returned_To_Position_On_Entry() {
    std::ifstream is(m_testAsciiPath.c_str(), std::ios::in | std::ios::binary);
    // move stream along one to check it is returned to here
    is.seekg(1);

    TS_ASSERT(FileDescriptor::isAscii(is));
    TS_ASSERT_EQUALS(1, is.tellg());
  }

  void test_isAscii_Returns_False_For_Stream_Pointing_At_Ascii_File_And_Stream_Is_Returned_To_Position_On_Entry() {
    std::ifstream is(m_testNonNexusPath.c_str(), std::ios::in | std::ios::binary);
    // move stream along one to check it is returned to here
    is.seekg(1);

    TS_ASSERT(!FileDescriptor::isAscii(is));
    TS_ASSERT_EQUALS(1, is.tellg());
  }

  void test_isAscii_Returns_True_For_C_Handle() {
    FILE *handle = fopen(m_testAsciiPath.c_str(), "r");
    if (handle) {
      TS_ASSERT(FileDescriptor::isAscii(handle));
      TS_ASSERT_EQUALS(0, ftell(handle));
      fclose(handle);
    }
  }

  void test_isAscii_Returns_False_For_C_Handle() {
    FILE *handle = fopen(m_testNonNexusPath.c_str(), "r");
    if (handle) {
      TS_ASSERT(!FileDescriptor::isAscii(handle));
      TS_ASSERT_EQUALS(0, ftell(handle));
      fclose(handle);
    }
  }

  void test_Constructor_With_Existing_File_Initializes_Description_Fields() {
    const std::string filename = m_testNexusPath;
    FileDescriptor descr(filename);

    TS_ASSERT_EQUALS(filename, descr.filename());
    TS_ASSERT_EQUALS(".nxs", descr.extension());
  }

  void test_Intial_Stream_Is_Positioned_At_Start_Of_File() {
    const std::string filename = m_testNexusPath;
    FileDescriptor descr(filename);

    auto &stream = descr.data();
    std::streamoff streamPos = stream.tellg();

    TS_ASSERT_EQUALS(0, streamPos);
  }

  void test_ResetStreamToStart_Repositions_Stream_Start_Of_File() {
    const std::string filename = m_testNexusPath;
    FileDescriptor descr(filename);
    auto &stream = descr.data();
    char byte('0');
    stream >> byte; // read byte from stream

    TS_ASSERT_EQUALS(1, stream.tellg());
    descr.resetStreamToStart();
    TS_ASSERT_EQUALS(0, stream.tellg());
  }

  void testEmptyFile() {
    TS_ASSERT_EQUALS(false, FileDescriptor::isEmpty(m_testAsciiPath));
    TS_ASSERT(FileDescriptor::isEmpty(m_emptyFilePath));
  }

  //===================== Failure cases
  //============================================
  void test_IsAscii_Throws_For_Inaccessible_Filename() {
    TS_ASSERT_THROWS(FileDescriptor::isAscii(""), const std::invalid_argument &);
    TS_ASSERT_THROWS(FileDescriptor::isAscii("__not_a_File.txt__"), const std::invalid_argument &);
  }

  void test_IsAscii_Returns_True_For_Ascii_Stream_Shorter_Than_NBytes_Requested_And_Clears_Error_Flags() {
    // Fake data
    std::istringstream is;
    is.str("abcdef"); // 6 bytes

    TS_ASSERT(FileDescriptor::isAscii(is, 6)); // Equal to length
    TS_ASSERT_EQUALS(0, is.tellg());
    TS_ASSERT(is.good());

    TS_ASSERT(FileDescriptor::isAscii(is, 10)); // Larger
    TS_ASSERT_EQUALS(0, is.tellg());
    TS_ASSERT(is.good());
  }

  void test_Constructor_Throws_With_Empty_filename() {
    TS_ASSERT_THROWS(FileDescriptor(""), const std::invalid_argument &);
  }

  void test_Constructor_Throws_With_NonExistant_filename() {
    TS_ASSERT_THROWS(FileDescriptor("__ThisShouldBeANonExistantFile.txt"), const std::invalid_argument &);
  }

  void testIsEmptyThrowsForInaccessibleFileName() {
    TS_ASSERT_THROWS(FileDescriptor::isEmpty(""), const std::invalid_argument &);
    TS_ASSERT_THROWS(FileDescriptor::isEmpty("__not_a_File.txt__"), const std::invalid_argument &);
  }

private:
  std::string m_testNexusPath;
  std::string m_testNonNexusPath;
  std::string m_testAsciiPath;
  std::string m_emptyFilePath;
};
