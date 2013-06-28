#ifndef MANTID_KERNEL_HDFDESCRIPTORTEST_H_
#define MANTID_KERNEL_HDFDESCRIPTORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/HDFDescriptor.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <Poco/Path.h>
#include <Poco/File.h>

#include <cstdio>

using Mantid::Kernel::HDFDescriptor;

class HDFDescriptorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HDFDescriptorTest *createSuite() { return new HDFDescriptorTest(); }
  static void destroySuite( HDFDescriptorTest *suite ) { delete suite; }


  HDFDescriptorTest()
  {
    using Mantid::Kernel::ConfigService;
    auto dataPaths = ConfigService::Instance().getDataSearchDirs();
    for(auto it = dataPaths.begin(); it != dataPaths.end(); ++it)
    {
      Poco::Path hdf5Path(*it, "CNCS_7860_event.nxs");
      if(Poco::File(hdf5Path).exists()) m_testHDF5Path = hdf5Path.toString();

      Poco::Path hdf4Path(*it, "argus0026287.nxs");
      if(Poco::File(hdf4Path).exists()) m_testHDF4Path = hdf4Path.toString();

      Poco::Path nonhdf5Path(*it, "CSP79590.raw");
      if(Poco::File(nonhdf5Path).exists()) m_testNonHDFPath = nonhdf5Path.toString();

      if(!m_testHDF5Path.empty() && !m_testHDF4Path.empty() && !m_testNonHDFPath.empty()) break;
    }
    if(m_testHDF5Path.empty() || m_testHDF4Path.empty() || m_testNonHDFPath.empty())
    {
      throw std::runtime_error("Unable to find test files for FileDescriptorTest. "
          "The AutoTestData directory needs to be in the search path");
    }

    m_testHDF5 = boost::make_shared<HDFDescriptor>(m_testHDF5Path);
  }

  //=================================== Static isHDF methods ======================================
  void test_isHDF_Returns_False_For_Non_HDF_Filename()
  {
    TS_ASSERT(!HDFDescriptor::isHDF(m_testNonHDFPath));
    TS_ASSERT(!HDFDescriptor::isHDF(m_testNonHDFPath, HDFDescriptor::AnyVersion));
    TS_ASSERT(!HDFDescriptor::isHDF(m_testNonHDFPath, HDFDescriptor::Version4));
    TS_ASSERT(!HDFDescriptor::isHDF(m_testNonHDFPath, HDFDescriptor::Version5));
  }

  void test_isHDF_Defaults_To_All_Versions()
  {
    TS_ASSERT(HDFDescriptor::isHDF(m_testHDF4Path));
    TS_ASSERT(HDFDescriptor::isHDF(m_testHDF5Path));
  }

  void test_isHDF_With_Version4_Returns_True_Only_For_HDF4()
  {
    TS_ASSERT(HDFDescriptor::isHDF(m_testHDF4Path, HDFDescriptor::Version4));
    TS_ASSERT(!HDFDescriptor::isHDF(m_testHDF5Path, HDFDescriptor::Version4));
  }

  void test_isHDF_With_Version5_Returns_True_Only_For_HDF4()
  {
    TS_ASSERT(HDFDescriptor::isHDF(m_testHDF5Path, HDFDescriptor::Version5));
    TS_ASSERT(!HDFDescriptor::isHDF(m_testHDF4Path, HDFDescriptor::Version5));
  }

  void test_isHDF_Throws_With_Invalid_Filename()
  {
    TS_ASSERT_THROWS(HDFDescriptor::isHDF(""), std::invalid_argument);
  }

  //=================================== HDFDescriptor methods ==================================

  void test_Constructor_Initializes_Object_Correctly_Given_HDF_File()
  {
    TS_ASSERT_EQUALS(m_testHDF5Path, m_testHDF5->filename());
    TS_ASSERT_EQUALS(".nxs", m_testHDF5->extension());
  }

  void test_Constructor_Throws_With_Empty_filename()
  {
    TS_ASSERT_THROWS(HDFDescriptor(""), std::invalid_argument);
  }

  void test_Constructor_Throws_With_NonExistant_filename()
  {
    TS_ASSERT_THROWS(HDFDescriptor("__ThisShouldBeANonExistantFile.txt"), std::invalid_argument);
  }

  void test_Constructor_Throws_When_Given_File_Not_Identified_As_HDF()
  {
    TS_ASSERT_THROWS(HDFDescriptor fd(m_testNonHDFPath), std::invalid_argument);
  }

  void test_PathExists_Returns_False_For_Path_Not_In_File()
  {
    TS_ASSERT(!m_testHDF5->pathExists("/raw_data_1/bank1"));
  }

  void test_PathExists_Returns_False_For_Invalid_Path_Specification()
  {
    TS_ASSERT(!m_testHDF5->pathExists("raw_data_1\\bank1"));
  }

  void test_PathExists_Returns_False_For_Root_Path_Along()
  {
    TS_ASSERT(!m_testHDF5->pathExists("/"));
  }

  void test_PathExists_Returns_True_For_Path_At_Any_Level_In_File()
  {
    TS_ASSERT(m_testHDF5->pathExists("/entry"));
    TS_ASSERT(m_testHDF5->pathExists("/entry/bank1/data_x_y"));
  }

  void test_classTypeExists_Returns_True_For_Type_At_Any_Level_In_File()
  {
    TS_ASSERT(m_testHDF5->classTypeExists("NXentry"));
    TS_ASSERT(m_testHDF5->classTypeExists("NXevent_data"));
    TS_ASSERT(m_testHDF5->classTypeExists("NXlog"));
  }

private:

  std::string m_testHDF5Path;
  std::string m_testHDF4Path;
  std::string m_testNonHDFPath;
  boost::shared_ptr<HDFDescriptor> m_testHDF5;
};


#endif /* MANTID_KERNEL_HDFDESCRIPTORTEST_H_ */
