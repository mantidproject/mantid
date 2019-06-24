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
#include <H5Object.h>

using namespace Mantid::NexusGeometry;

//---------------------------------------------------------------
namespace {

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
};

class HDF5FileTestUtility {

public:
  HDF5FileTestUtility(const std::string &fullPath)
      : m_file(fullPath, H5F_ACC_RDONLY) {}

  bool hasNxClass(const std::string &classType, const std::string &path) const {

    H5::Group parentGroup = m_file.openGroup(path);
    H5::Attribute attribute = parentGroup.openAttribute("NX_class");

    std::string readClass;
    attribute.read(attribute.getDataType(), readClass);
    return (readClass == classType) ? true
                                    : false; // check if NXclass exists in group
  }

private:
  H5::H5File m_file;
};

// Gives a clean file destination and removes after if present.

class ScopedFileHandle {

public:
  ScopedFileHandle(const std::string &name) {

    
    auto temp_dir = boost::filesystem::temp_directory_path();
    auto temp_full_path = temp_dir /= name;

    // Check proposed location and throw std::invalid argument if file does not
    // exist. otherwise set m_full_path to location.
    (boost::filesystem::exists(temp_full_path))
        ? m_full_path = temp_full_path
        : throw std::invalid_argument("file does not exist.");
  }

  std::string fullPath() const {
    return m_full_path.generic_string();
  }

  ~ScopedFileHandle() {
    // TODO delete the file.
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
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusGeometrySaveTest *createSuite() {
    return new NexusGeometrySaveTest();
  }
  static void destroySuite(NexusGeometrySaveTest *suite) { delete suite; }

  void test_providing_invalid_path_throws() {

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        Mantid::Kernel::V3D(1, 1, 1));

    auto inst2 = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    std::string path = "invalid_path"; // test valid path

    TS_ASSERT_THROWS(saveInstrument(*inst2.first, path),
                     std::invalid_argument &);
  }

  void test_progress_reporting() {
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        Mantid::Kernel::V3D(1, 1, 1));

    auto inst2 = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    MockProgressBase progressRep;
    EXPECT_CALL(progressRep, doReport(testing::_)).Times(1);
    std::string path = "C:\\Users\\mqi61253"; // test valid path
    saveInstrument(*inst2.first, path, &progressRep);
    ASSERT_TRUE(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  // WIP-----------------------------------------------------

  void test_nxinstrument_class_exists() {

    // Instrument--------------------------------------
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        Mantid::Kernel::V3D(1, 1, 1));

    auto inst2 = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    //--------------------------------------------------------------------

    // destination folder for outputfile-----------------------------------

    /*
    ScopedFileHandle fileResource("WISH_Definition_10Panels.hdf5");
        destinationFile = fileResource.fullPath();
    */

    std::string destinationFile =
        "C:\\Users\\mqi61253\\WISH_Definition_10Panels.hdf5"; // some path to
                                                              // save the hdf5
                                                              // file

    // Check file itself.

    // saveInstrument(*inst2.first,
    //              destinationFile); //, progress); <-optional pointer

    HDF5FileTestUtility tester(destinationFile);

    // RAII ScopedFileHanle

    /*
    class that takes hdf5 file and tests that is has
    attributes and classes (see provided links) among
    other things
        */

    ASSERT_TRUE(tester.hasNxClass(
        "NXinstrument", "/raw_data_1/instrument")); // goto arg1 in hdf5
    //      file and check if arg0 exists
  }
};

/*
bool H5::H5Object::attrExists(const H5std_string &name)const <= check if
attribute exists int H5::H5Object::getNumAttrs()const <= get number of
attributes H5std_string H5::H5Object::getObjName()const <= return object name as
string.
*/

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
