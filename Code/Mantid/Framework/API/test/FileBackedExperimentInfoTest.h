#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidAPI/FileFinder.h"

#include <nexus/NeXusFile.hpp>

using Mantid::API::FileBackedExperimentInfo;

class FileBackedExperimentInfoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileBackedExperimentInfoTest *createSuite() { return new FileBackedExperimentInfoTest(); }
  static void destroySuite( FileBackedExperimentInfoTest *suite ) { delete suite; }

  FileBackedExperimentInfoTest()
  {
    using Mantid::API::ExperimentInfo;
    using Mantid::API::FileFinder;
    // Cache in-memory experiment info for comparison
    m_filename = FileFinder::Instance().getFullPath("TOPAZ_3680_5_sec_MDEW.nxs");
    if(m_filename.empty())
    {
      throw std::runtime_error("Cannot find test file TOPAZ_3680_5_sec_MDEW.nxs");
    }
    
    m_inMemoryExptInfo = boost::make_shared<ExperimentInfo>();
   ::NeXus::File nexusFile(m_filename, NXACC_READ);
    nexusFile.openGroup("MDEventWorkspace", "NXentry");
    nexusFile.openGroup("experiment0", "NXgroup");
    std::string paramString;
    m_inMemoryExptInfo->loadExperimentInfoNexus(&nexusFile, paramString);
    m_inMemoryExptInfo->readParameterMap(paramString);
  }
  
  void test_toString_method_returns_same_as_ExperimentInfo_class()
  {
    // Load the file we want to use
    ::NeXus::File nexusFile(m_filename, NXACC_READ);

    // Create the file backed experiment info, shouldn't be loaded yet
    FileBackedExperimentInfo fileBacked(&nexusFile, "/MDEventWorkspace/experiment0");

    TS_ASSERT_EQUALS(fileBacked.toString(), m_inMemoryExptInfo->toString());
  }

  //------------------------------------------------------------------------------------------------a
  // Failure tests
  //------------------------------------------------------------------------------------------------
  void test_runtime_error_generated_when_unable_to_load_from_file()
  {
    // Load the file we want to use
    ::NeXus::File nexusFile(m_filename, NXACC_READ);

    // Create the file backed experiment info, shouldn't be loaded yet
    FileBackedExperimentInfo fileBacked(&nexusFile, "/not/right/path");
    
    TS_ASSERT_THROWS(fileBacked.toString(), std::runtime_error);

  }
  
private:
  std::string m_filename;
  Mantid::API::ExperimentInfo_sptr m_inMemoryExptInfo;
};


#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_ */
