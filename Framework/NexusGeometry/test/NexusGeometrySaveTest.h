// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/filesystem.hpp>

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <gmock/gmock.h>

#include "H5cpp.h"
#include <H5DataSet.h>
#include <H5File.h>
#include <H5Group.h>
#include <H5Location.h>
#include <H5Object.h>
#include <H5Rpublic.h>

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
};

//  local Class used for validation of the structure of a nexus file as needed
//  for the unit tests.
class HDF5FileTestUtility {

public:
  HDF5FileTestUtility(const std::string &fullPath) {
    boost::filesystem::path tmp = fullPath;
    if (!boost::filesystem::exists(tmp)) {
      throw std::invalid_argument("no such file.\n");
    } else {
      m_file.openFile(fullPath, H5F_ACC_RDONLY);
    }
  }

  bool hasNxClass(const std::string &attrVal, const std::string &path) const {

    H5::Group parentGroup = m_file.openGroup(path);
    H5::Attribute attribute = parentGroup.openAttribute("NX_class");

    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);
    return attributeValue == attrVal;
  }

  bool hasDataSet(const std::string name, const std::string &attrName,
                  const std::string &attrVal, const std::string &path) const {

    H5::Group parentGroup =
        m_file.openGroup(path); // open group where DataSet should exist.

    try {

      // dataset with name 'name' exists if try block succeeds.
      H5::DataSet dataSet = parentGroup.openDataSet(name);

      // attribute of dataset with atrtibute name 'attrName' exists if try block
      // succeeds.
      H5::Attribute attribute = dataSet.openAttribute(attrName);
      std::string readClass;
      attribute.read(attribute.getDataType(), readClass);

      // if value of attribute read from file matches attrval, returns true.
      return readClass == attrVal;

    } catch (H5::Exception &) {

      // DataSet could not be opened.
      return false;
    }
  }

private:
  H5::H5File m_file;
};

// Gives a clean file destination and removes the file when
// handle is out of scope. Must be stack allocated.
class ScopedFileHandle {

public:
  ScopedFileHandle(const std::string &fileName) {

    const auto temp_dir = boost::filesystem::temp_directory_path();
    auto temp_full_path = temp_dir;
    temp_full_path /= fileName;

    // Check proposed location and throw std::invalid argument if file does
    // not exist. otherwise set m_full_path to location.

    if (boost::filesystem::is_directory(temp_dir)) {
      m_full_path = temp_full_path;

    } else {
      throw std::invalid_argument("failed to load temp directory: " +
                                  temp_dir.generic_string());
    }
  }

  std::string fullPath() const { return m_full_path.generic_string(); }

  ~ScopedFileHandle() {

    // file is removed at end of file handle's lifetime
    if (boost::filesystem::is_regular_file(m_full_path)) {
      boost::filesystem::remove(m_full_path);
    }
  }

private:
  boost::filesystem::path m_full_path; // full path to file
};

} // namespace

//---------------------------------------------------------------------

class NexusGeometrySaveTest : public CxxTest::TestSuite {

private:
  std::pair<std::unique_ptr<Mantid::Geometry::ComponentInfo>,
            std::unique_ptr<Mantid::Geometry::DetectorInfo>>
      m_instrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometrySaveTest *createSuite() {
    return new NexusGeometrySaveTest();
  }
  static void destroySuite(NexusGeometrySaveTest *suite) { delete suite; }

  NexusGeometrySaveTest() {
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        Mantid::Kernel::V3D(1, 1, 1));

    m_instrument =
        Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
  }

  void test_providing_invalid_path_throws() { // deliberately fails

    ScopedFileHandle destinationFile("invalid_path_to_file_test_file.hdf5");
    std::string path = "false_directory\\" + destinationFile.fullPath();

    TS_ASSERT_THROWS(saveInstrument(*m_instrument.first, path),
                     std::invalid_argument &);
  }

  void test_progress_reporting() {

    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(1);

    ScopedFileHandle destinationFile("progress_report_test_file.hdf5");
    std::string path = destinationFile.fullPath();

    saveInstrument(*m_instrument.first, path, &progressRep);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  void test_extension_validation() {

    ScopedFileHandle fileResource(
        "invalid_extension_test_file.abc"); // creates a temp directory for the
                                            // file.
    std::string destinationFile = fileResource.fullPath();

    TS_ASSERT_THROWS(saveInstrument(*m_instrument.first, destinationFile),
                     std::invalid_argument &);
  }

  void test_nxinstrument_class_exists() {

    ScopedFileHandle fileResource(
        "check_instrument_test_file.hdf5"); // creates a temp directory for the
                                            // file.
    std::string destinationFile = fileResource.fullPath();
    saveInstrument(*m_instrument.first,
                   destinationFile); // saves the instrument.

    HDF5FileTestUtility tester(
        destinationFile); // tests the file has given attributes.

    TS_ASSERT(tester.hasNxClass("NXinstrument", "/raw_data_1/instrument"));
    TS_ASSERT(tester.hasNxClass("NXentry", "/raw_data_1"));
  }

  void test_instrument_has_name() {

    ScopedFileHandle scopedFile(
        "check_instrument_name_test_file.hdf5"); // temp directory
    auto destinationFile = scopedFile.fullPath();

    saveInstrument(*m_instrument.first, destinationFile); // saves instrument
    HDF5FileTestUtility testUtility(destinationFile);

    TS_ASSERT(testUtility.hasDataSet("name", "short_name", "abr_name",
                                     "/raw_data_1/instrument"));

    TS_ASSERT(testUtility.hasDataSet("name", "NX_class", "NX_CHAR",
                                     "/raw_data_1/instrument"));
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
