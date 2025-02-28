// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LegacyNexusDescriptor.h"
#include "MantidLegacyNexus/NeXusFile.hpp"
#include <cxxtest/TestSuite.h>

#include <filesystem>
#include <memory>

#include <cstdio>

using Mantid::Kernel::LegacyNexusDescriptor;

class LegacyNexusDescriptorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LegacyNexusDescriptorTest *createSuite() { return new LegacyNexusDescriptorTest(); }
  static void destroySuite(LegacyNexusDescriptorTest *suite) { delete suite; }

  LegacyNexusDescriptorTest() {
    using Mantid::Kernel::ConfigService;
    auto dataPaths = ConfigService::Instance().getDataSearchDirs();
    for (auto &dataPath : dataPaths) {
      const auto hdf5Path = std::filesystem::path(dataPath) / "CNCS_7860_event.nxs";
      if (std::filesystem::exists(hdf5Path))
        m_testHDF5Path = hdf5Path.string();

      const auto nonhdf5Path = std::filesystem::path(dataPath) / "CSP79590.raw";
      if (std::filesystem::exists(nonhdf5Path))
        m_testNonHDFPath = nonhdf5Path.string();

      if (!m_testHDF5Path.empty() && !m_testNonHDFPath.empty())
        break;
    }
    if (m_testHDF5Path.empty() || m_testNonHDFPath.empty()) {
      throw std::runtime_error("Unable to find test files for FileDescriptorTest. "
                               "The AutoTestData directory needs to be in the search path");
    }

    m_testHDF5 = std::make_shared<LegacyNexusDescriptor>(m_testHDF5Path);
  }

  //=================================== LegacyNexusDescriptor methods
  //==================================

  void test_Constructor_Initializes_Object_Correctly_Given_HDF_File() {
    TS_ASSERT_EQUALS(m_testHDF5Path, m_testHDF5->filename());
    TS_ASSERT_EQUALS(".nxs", m_testHDF5->extension());
  }

  void test_Constructor_Throws_With_Empty_filename() {
    TS_ASSERT_THROWS(LegacyNexusDescriptor(""), const std::invalid_argument &);
  }

  void test_Constructor_Throws_With_NonExistant_filename() {
    TS_ASSERT_THROWS(LegacyNexusDescriptor("__ThisShouldBeANonExistantFile.txt"), const std::invalid_argument &);
  }

  void test_Constructor_Throws_When_Given_File_Not_Identified_As_HDF() {
    TS_ASSERT_THROWS(LegacyNexusDescriptor fd(m_testNonHDFPath), const std::invalid_argument &);
  }

  void test_File_Handle_Returned_By_Data_Is_Valid() {
    auto &file = m_testHDF5->data();
    TS_ASSERT_EQUALS("", file.getPath())
  }

  void test_firstEntryNameType_Returns_Correct_Details() {
    auto entryType = m_testHDF5->firstEntryNameType();
    TS_ASSERT_EQUALS("entry", entryType.first);
    TS_ASSERT_EQUALS("NXentry", entryType.second);
  }

  void test_PathExists_Returns_False_For_Path_Not_In_File() { TS_ASSERT(!m_testHDF5->pathExists("/raw_data_1/bank1")); }

  void test_PathExists_Returns_False_For_Invalid_Path_Specification() {
    TS_ASSERT(!m_testHDF5->pathExists("raw_data_1\\bank1"));
  }

  void test_PathExists_Returns_False_For_Root_Path_Along() { TS_ASSERT(!m_testHDF5->pathExists("/")); }

  void test_PathExists_Returns_True_For_Path_At_Any_Level_In_File() {
    TS_ASSERT(m_testHDF5->pathExists("/entry"));
    TS_ASSERT(m_testHDF5->pathExists("/entry/bank1/data_x_y"));
  }

private:
  std::string m_testHDF5Path;
  std::string m_testNonHDFPath;
  std::shared_ptr<LegacyNexusDescriptor> m_testHDF5;
};
