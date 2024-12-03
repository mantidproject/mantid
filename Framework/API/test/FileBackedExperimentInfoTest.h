// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ExperimentInfoTest.h"
#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidNexusCpp/NeXusFile.hpp"
#include <cxxtest/TestSuite.h>

using Mantid::API::FileBackedExperimentInfo;

class FileBackedExperimentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileBackedExperimentInfoTest *createSuite() { return new FileBackedExperimentInfoTest(); }
  static void destroySuite(FileBackedExperimentInfoTest *suite) { delete suite; }

  FileBackedExperimentInfoTest() {
    using Mantid::API::ExperimentInfo;
    using Mantid::API::FileFinder;
    // Cache in-memory experiment info for comparison
    m_filename = FileFinder::Instance().getFullPath("HRP38692a.nxs");
    if (m_filename.empty()) {
      throw std::runtime_error("Cannot find test file HRP38692a.nxs");
    }

    m_inMemoryExptInfo = std::make_shared<ExperimentInfo>();
    ::NeXus::File nxFile(m_filename, NXACC_READ);
    nxFile.openGroup("mantid_workspace_1", "NXentry");
    std::string paramString;
    m_inMemoryExptInfo->loadExperimentInfoNexus(m_filename, &nxFile, paramString);
    m_inMemoryExptInfo->readParameterMap(paramString);
  }

  void test_toString_populates_object() {
    auto fileBacked = createTestObject();
    TS_ASSERT_EQUALS(fileBacked->toString(), m_inMemoryExptInfo->toString());
  }

  void test_cloneExperimentInfo_returns_new_file_backed_object_and_does_not_touch_file() {
    auto fileBacked = createTestObject();
    auto *clonedFileBacked = fileBacked->cloneExperimentInfo();

    TS_ASSERT(dynamic_cast<FileBackedExperimentInfo *>(clonedFileBacked));

    delete clonedFileBacked;
  }

  void test_getInstrument_populates_object() {
    auto fileBacked = createTestObject();

    TS_ASSERT_EQUALS(fileBacked->constInstrumentParameters(), m_inMemoryExptInfo->constInstrumentParameters());
  }

  void test_instrumentParameters_const_ref_method_populate_object() {
    auto fileBacked = createTestObject();
    const auto &pmap = fileBacked->instrumentParameters(); // const

    TS_ASSERT(pmap.size() > 0);
  }

  void test_nonconst_ref_instrumentParameters_method_populate_object() {
    auto fileBacked = createTestObject();
    auto &pmap = fileBacked->instrumentParameters(); // non-const

    TS_ASSERT(pmap.size() > 0);
  }

  void test_constInstrumentParameters_method_populate_object() {
    auto fileBacked = createTestObject();
    const auto &pmap = fileBacked->constInstrumentParameters();

    TS_ASSERT(pmap.size() > 0);
  }

  void test_populateInstrumentParameters_method_populate_object() {
    auto fileBacked = createTestObject();
    fileBacked->populateInstrumentParameters();
    const auto &pmap = fileBacked->constInstrumentParameters();

    TS_ASSERT(pmap.size() > 0);
    for (auto param : pmap)
      TS_ASSERT_EQUALS((param.second)->visible(), true);
  }

  void test_sample() {
    auto fileBacked = createTestObject();

    TS_ASSERT_EQUALS(m_inMemoryExptInfo->sample().getGeometryFlag(), fileBacked->sample().getGeometryFlag());
    TS_ASSERT_EQUALS(m_inMemoryExptInfo->sample().getGeometryFlag(), fileBacked->mutableSample().getGeometryFlag());
  }

  void test_run() {
    auto fileBacked = createTestObject();

    TS_ASSERT_EQUALS(m_inMemoryExptInfo->run().getProtonCharge(), fileBacked->run().getProtonCharge())
  }

  void test_getLog() {
    auto fileBacked = createTestObject();

    TS_ASSERT_EQUALS(m_inMemoryExptInfo->getLogAsSingleValue("gd_prtn_chrg"),
                     fileBacked->getLogAsSingleValue("gd_prtn_chrg"));

    auto *inMemoryProp = m_inMemoryExptInfo->getLog("gd_prtn_chrg");
    auto *fileBackedProp = fileBacked->getLog("gd_prtn_chrg");
    TS_ASSERT_EQUALS(inMemoryProp->value(), fileBackedProp->value());
  }

  void test_getRunNumber() {
    auto fileBacked = createTestObject();

    TS_ASSERT_EQUALS(m_inMemoryExptInfo->getRunNumber(), fileBacked->getRunNumber());
  }

  void test_getEMode() {
    auto fileBacked = createTestObject();

    TS_ASSERT_EQUALS(m_inMemoryExptInfo->getEMode(), fileBacked->getEMode());
  }

  void test_getEFixed() {
    auto fileBacked = createTestObject();

    TS_ASSERT_THROWS(fileBacked->getEFixed(10100), const std::runtime_error &);
  }

  void test_setEFixed() {
    auto fileBacked = createTestObject();

    TS_ASSERT_THROWS_NOTHING(fileBacked->setEFixed(10100, 12.5));
  }

  //------------------------------------------------------------------------------------------------
  // Failure tests
  //------------------------------------------------------------------------------------------------
  void test_runtime_error_generated_when_unable_to_load_from_file() {
    // Create the file backed experiment info, shouldn't be loaded yet
    FileBackedExperimentInfo fileBacked(m_filename, "/not/right/path");

    TS_ASSERT_THROWS(fileBacked.toString(), const std::runtime_error &);
  }

private:
  Mantid::API::ExperimentInfo_sptr createTestObject() {
    // Create the file backed experiment info, shouldn't be loaded yet.
    // Manipulate it through the interface
    return std::make_shared<FileBackedExperimentInfo>(m_filename, "/mantid_workspace_1");
  }

  Mantid::API::ExperimentInfo_sptr m_inMemoryExptInfo;
  std::string m_filename;
};
