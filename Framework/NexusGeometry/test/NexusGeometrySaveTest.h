// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_
#define MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <gmock/gmock.h>
#include <iostream>

#include "H5cpp.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::NexusGeometry;

namespace {

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
};

} // namespace

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

    std::string path = "invalid_path"; // valid path

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
    std::string path = "C:\\Users\\mqi61253"; // valid path
    saveInstrument(*inst2.first, path, &progressRep);
    ASSERT_TRUE(testing::Mock::VerifyAndClearExpectations(&progressRep));
  }

  // WIP-----------------------------------------------------

  void test_nxinstrument_class_exists() {}

  /*
    // Instrument--------------------------------------
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        Mantid::Kernel::V3D(1, 1, 1));

    auto inst2 = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
    //--------------------------------------------------------------------

    // destination folder for outputfile-----------------------------------
    std::string destinationFile = ""; // some path to save the hdf5 file

    saveInstrument(*inst2.first,
                   destinationFile); //, progress); <-optional pointer

    // Check file itself.
    HDF5FileTestUtility tester(
        destinationFile); // class that takes hdf5 file and tests that is has
                          // attributes and classes (see provided links) among
                          // other things

//    ASSERT_TRUE(tester.hasNxClass(
  //      "NXinstrument", "/raw_data_1/instrument")); // goto arg1 in hdf5 file
                                                    // and check if arg0 exists
  }
};



*/



};

// WIP
class HDF5FileTestUtility {

private:
  std::string destination;

public:
  HDF5FileTestUtility(std::string inputPath);

  bool hasNxClass(std::string className, std::string HDF5Path) const;
  int numOfAttr() const;
};

/*
bool H5::H5Object::attrExists(const H5std_string &name)const <= check if
attribute exists int H5::H5Object::getNumAttrs()const <= get number of
attributes H5std_string H5::H5Object::getObjName()const <= return object name as
string.

*/
#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */