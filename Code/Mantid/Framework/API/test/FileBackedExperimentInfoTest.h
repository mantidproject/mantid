#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidAPI/FileFinder.h"

#include <nexus/NeXusFile.hpp>

using Mantid::API::FileBackedExperimentInfo;

class FileBackedExperimentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileBackedExperimentInfoTest *createSuite() {
    return new FileBackedExperimentInfoTest();
  }
  static void destroySuite(FileBackedExperimentInfoTest *suite) {
    delete suite;
  }

  FileBackedExperimentInfoTest() {
    using Mantid::API::ExperimentInfo;
    using Mantid::API::FileFinder;
    // Cache in-memory experiment info for comparison
    m_filename = FileFinder::Instance().getFullPath("HRP38692a.nxs");
    if (m_filename.empty()) {
      throw std::runtime_error("Cannot find test file HRP38692a.nxs");
    }

    m_inMemoryExptInfo = boost::make_shared<ExperimentInfo>();
    m_nexusFile = boost::make_shared<::NeXus::File>(m_filename, NXACC_READ);
    m_nexusFile->openGroup("mantid_workspace_1", "NXentry");
    std::string paramString;
    m_inMemoryExptInfo->loadExperimentInfoNexus(m_nexusFile.get(), paramString);
    m_inMemoryExptInfo->readParameterMap(paramString);
  }

  void test_toString_populates_object() {
    auto fileBacked = createTestObject();
    TS_ASSERT_EQUALS(fileBacked->toString(), m_inMemoryExptInfo->toString());
  }

  void test_cloneExperimentInfo_populates_object() {
    auto fileBacked = createTestObject();
    auto *clonedFileBacked = fileBacked->cloneExperimentInfo();

    TS_ASSERT_EQUALS(clonedFileBacked->toString(),
                     m_inMemoryExptInfo->toString());
    delete clonedFileBacked;
  }

  void test_getInstrument_populates_object() {
    auto fileBacked = createTestObject();
    auto fileBackedInstrument = fileBacked->getInstrument();
    auto inMemoryInstrument = m_inMemoryExptInfo->getInstrument();

    TS_ASSERT_EQUALS(fileBacked->constInstrumentParameters(),
                     m_inMemoryExptInfo->constInstrumentParameters());
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

  //------------------------------------------------------------------------------------------------
  // Failure tests
  //------------------------------------------------------------------------------------------------
  void test_runtime_error_generated_when_unable_to_load_from_file() {
    // Load the file we want to use
    ::NeXus::File nexusFile(m_filename, NXACC_READ);

    // Create the file backed experiment info, shouldn't be loaded yet
    FileBackedExperimentInfo fileBacked(&nexusFile, "/not/right/path");

    TS_ASSERT_THROWS(fileBacked.toString(), std::runtime_error);
  }

private:
  Mantid::API::ExperimentInfo_sptr createTestObject() {
    // Load the file we want to use
    ::NeXus::File nexusFile(m_filename, NXACC_READ);
    // Create the file backed experiment info, shouldn't be loaded yet.
    // Manipulate it through
    // the interface
    return boost::make_shared<FileBackedExperimentInfo>(m_nexusFile.get(),
                                                        "/mantid_workspace_1");
  }

  boost::shared_ptr<::NeXus::File> m_nexusFile;
  Mantid::API::ExperimentInfo_sptr m_inMemoryExptInfo;
  std::string m_filename;
};

#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_ */
