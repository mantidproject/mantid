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

#include <H5Cpp.h>

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

// NEXUS COMPLIANT ATTRIBUTE NAMES
const H5std_string SHORT_NAME = "short_name";
const H5std_string NX_CLASS = "NX_class";
const H5std_string NX_ENTRY = "NXentry";
const H5std_string NX_INSTRUMENT = "NXinstrument";
const H5std_string NX_SOURCE = "NXsource";
const H5std_string SHAPE = "shape";

// NEXUS COMPLIANT ATTRIBUTE VALUES
const H5std_string NX_TRANSFORMATION = "NXtransformation";
const H5std_string NX_CHAR = "NX_CHAR";

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

  // check if group has matching NX_class attribute value, or if dataype has
  // matching NX_class attribute value if optional dataset name passed.
  bool hasNxClass(const std::string &attrVal, const std::string &pathToGroup,
                  std::string *dataSetname = nullptr) const {

    H5::Attribute attribute;

    // group must be opened before accessing dataset
    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    if (dataSetname != nullptr) {

      // treat as dataset

      H5::DataSet dataSet = parentGroup.openDataSet(*dataSetname);
      attribute = dataSet.openAttribute(NX_CLASS);

    } else {

      // treat as group
      attribute = parentGroup.openAttribute(NX_CLASS);
    }

    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
  }

  // check if dataset exists in group, and (optional) check if the dataset has
  // the NX_class attribute specified.

  bool hasDataSet(const std::string &dataSetName,
                  const std::string &pathToGroup,
                  const std::string *nxClassAttrVal = nullptr) const {

    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    try {
      H5::DataSet dataSet = parentGroup.openDataSet(dataSetName);

      if (nxClassAttrVal != nullptr) {

        H5::Attribute attribute = dataSet.openAttribute(NX_CLASS);
        std::string readClass;

        attribute.read(attribute.getDataType(), readClass);
        return readClass == *nxClassAttrVal;
      } else {

        // return true if dataset opened without exception and without checking
        // for NX_class attribute.
        return true;
      }

    } catch (H5::DataSetIException) {
      return false;
    }
  }

  // check if dataset or group has name-specific attribute
  bool hasAttribute(const std::string &pathToGroup, const std::string attrName,
                    const std::string attrVal,
                    const std::string *dataSetname = nullptr) {

    H5::Attribute attribute;

    // group must be opened before accessing dataset
    H5::Group parentGroup = m_file.openGroup(pathToGroup);

    if (dataSetname != nullptr) {

      // treat as dataset

      H5::DataSet dataSet = parentGroup.openDataSet(*dataSetname);
      attribute = dataSet.openAttribute(attrName);

    } else {

      // treat as group
      attribute = parentGroup.openAttribute(attrName);
    }

    std::string attributeValue;
    attribute.read(attribute.getDataType(), attributeValue);

    return attributeValue == attrVal;
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

  // prevent heap allocation for ScopedFileHandle
protected:
  static void *operator new(std::size_t); // prevent heap allocation of scalar.
  static void *operator new[](std::size_t); // prevent heap allocation of array.
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

    std::string dataSetName = "name";

    TS_ASSERT(tester.hasNxClass(
        NX_INSTRUMENT, "/raw_data_1/instrument")); // check child group has
                                                   // NX_class 'NX_INSTRUMENT'
    TS_ASSERT(tester.hasNxClass(
        NX_ENTRY, "/raw_data_1")); // check parent group has NX_class 'NX_ENTRY'

    TS_ASSERT(
        tester.hasNxClass(NX_CHAR, "/raw_data_1/instrument",
                          &dataSetName)) // check dataset has NX_class 'NX_CHAR'
  }

  void test_instrument_has_name() {

    ScopedFileHandle scopedFile(
        "check_instrument_name_test_file.hdf5"); // temp directory
    auto destinationFile = scopedFile.fullPath();

    auto const &compInfo = (*m_instrument.first);
    const std::string expectedInstrumentName = compInfo.name(compInfo.root());

    saveInstrument(compInfo, destinationFile); // saves instrument
    HDF5FileTestUtility testUtility(destinationFile);

    std::string dataSetName = "name";
    // expectedInstrumentName; // let dataset name have same value as component
    // name.

    TS_ASSERT(testUtility.hasDataSet(dataSetName, "/raw_data_1/instrument",
                                     &NX_CHAR));

    TS_ASSERT(testUtility.hasAttribute("/raw_data_1/instrument", "short_name",
                                       "name", &dataSetName));
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
