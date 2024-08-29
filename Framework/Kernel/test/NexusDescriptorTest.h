// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NexusDescriptor.h"
#include <cxxtest/TestSuite.h>

#include <filesystem>
#include <memory>

#include <nexus/NeXusFile.hpp>

#include <cstdio>

using Mantid::Kernel::NexusDescriptor;

class NexusDescriptorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusDescriptorTest *createSuite() { return new NexusDescriptorTest(); }
  static void destroySuite(NexusDescriptorTest *suite) { delete suite; }

  NexusDescriptorTest() {
    using Mantid::Kernel::ConfigService;
    auto dataPaths = ConfigService::Instance().getDataSearchDirs();
    for (auto &dataPath : dataPaths) {
      const auto hdf5Path = std::filesystem::path(dataPath) / "CNCS_7860_event.nxs";
      if (std::filesystem::exists(hdf5Path))
        m_testHDF5Path = hdf5Path.string();

      const auto hdf4Path = std::filesystem::path(dataPath) / "argus0026287.nxs";
      if (std::filesystem::exists(hdf4Path))
        m_testHDF4Path = hdf4Path.string();

      const auto nonhdf5Path = std::filesystem::path(dataPath) / "CSP79590.raw";
      if (std::filesystem::exists(nonhdf5Path))
        m_testNonHDFPath = nonhdf5Path.string();

      if (!m_testHDF5Path.empty() && !m_testHDF4Path.empty() && !m_testNonHDFPath.empty())
        break;
    }
    if (m_testHDF5Path.empty() || m_testHDF4Path.empty() || m_testNonHDFPath.empty()) {
      throw std::runtime_error("Unable to find test files for FileDescriptorTest. "
                               "The AutoTestData directory needs to be in the search path");
    }

    m_testHDF5 = std::make_shared<NexusDescriptor>(m_testHDF5Path);
  }

  //=================================== Static isReadable methods
  //======================================
  void test_isReadable_Returns_False_For_Non_HDF_Filename() {
    TS_ASSERT(!NexusDescriptor::isReadable(m_testNonHDFPath));
    TS_ASSERT(!NexusDescriptor::isReadable(m_testNonHDFPath, NexusDescriptor::AnyVersion));
    TS_ASSERT(!NexusDescriptor::isReadable(m_testNonHDFPath, NexusDescriptor::Version4));
    TS_ASSERT(!NexusDescriptor::isReadable(m_testNonHDFPath, NexusDescriptor::Version5));
  }

  void test_isReadable_Defaults_To_All_Versions() {
    TS_ASSERT(NexusDescriptor::isReadable(m_testHDF4Path));
    TS_ASSERT(NexusDescriptor::isReadable(m_testHDF5Path));
  }

  void test_isReadable_With_Version4_Returns_True_Only_For_HDF4() {
    TS_ASSERT(NexusDescriptor::isReadable(m_testHDF4Path, NexusDescriptor::Version4));
    TS_ASSERT(!NexusDescriptor::isReadable(m_testHDF5Path, NexusDescriptor::Version4));
  }

  void test_isReadable_With_Version5_Returns_True_Only_For_HDF4() {
    TS_ASSERT(NexusDescriptor::isReadable(m_testHDF5Path, NexusDescriptor::Version5));
    TS_ASSERT(!NexusDescriptor::isReadable(m_testHDF4Path, NexusDescriptor::Version5));
  }

  void test_isReadable_Throws_With_Invalid_Filename() {
    TS_ASSERT_THROWS(NexusDescriptor::isReadable(""), const std::invalid_argument &);
  }

  //=================================== NexusDescriptor methods
  //==================================

  void test_Constructor_Initializes_Object_Correctly_Given_HDF_File() {
    TS_ASSERT_EQUALS(m_testHDF5Path, m_testHDF5->filename());
    TS_ASSERT_EQUALS(".nxs", m_testHDF5->extension());
  }

  void test_Constructor_Throws_With_Empty_filename() {
    TS_ASSERT_THROWS(NexusDescriptor(""), const std::invalid_argument &);
  }

  void test_Constructor_Throws_With_NonExistant_filename() {
    TS_ASSERT_THROWS(NexusDescriptor("__ThisShouldBeANonExistantFile.txt"), const std::invalid_argument &);
  }

  void test_Constructor_Throws_When_Given_File_Not_Identified_As_HDF() {
    TS_ASSERT_THROWS(NexusDescriptor fd(m_testNonHDFPath), const std::invalid_argument &);
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

  void test_hasRootAttr_Returns_True_For_Existing_Attr() { TS_ASSERT(m_testHDF5->hasRootAttr("file_time")); }

  void test_hasRootAttr_Returns_False_For_Non_Existing_Attr() { TS_ASSERT(!m_testHDF5->hasRootAttr("not_attr")); }

  void test_PathExists_Returns_False_For_Path_Not_In_File() { TS_ASSERT(!m_testHDF5->pathExists("/raw_data_1/bank1")); }

  void test_PathExists_Returns_False_For_Invalid_Path_Specification() {
    TS_ASSERT(!m_testHDF5->pathExists("raw_data_1\\bank1"));
  }

  void test_PathExists_Returns_False_For_Root_Path_Along() { TS_ASSERT(!m_testHDF5->pathExists("/")); }

  void test_PathExists_Returns_True_For_Path_At_Any_Level_In_File() {
    TS_ASSERT(m_testHDF5->pathExists("/entry"));
    TS_ASSERT(m_testHDF5->pathExists("/entry/bank1/data_x_y"));
  }

  void test_PathOfTypeExists_Returns_True_For_Path_Of_Right_Type_At_Any_Level_In_File() {
    TS_ASSERT(m_testHDF5->pathOfTypeExists("/entry", "NXentry"));
    TS_ASSERT(m_testHDF5->pathOfTypeExists("/entry/bank1_events", "NXevent_data"));
  }

  void test_PathOfTypeExists_Returns_False_For_Path_In_File_But_Of_Wrong_Type() {
    TS_ASSERT(!m_testHDF5->pathOfTypeExists("/entry", "NXlog"));
    TS_ASSERT(!m_testHDF5->pathOfTypeExists("/entry/bank1_events", "NXentry"));
  }

  void test_classTypeExists_Returns_True_For_Type_At_Any_Level_In_File() {
    TS_ASSERT(m_testHDF5->classTypeExists("NXentry"));
    TS_ASSERT(m_testHDF5->classTypeExists("NXevent_data"));
    TS_ASSERT(m_testHDF5->classTypeExists("NXlog"));
  }

private:
  std::string m_testHDF5Path;
  std::string m_testHDF4Path;
  std::string m_testNonHDFPath;
  std::shared_ptr<NexusDescriptor> m_testHDF5;
};
