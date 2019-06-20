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
#include <iostream>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::NexusGeometry;

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

  void test_providing_valid_path_throw_fails() {

    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        Mantid::Kernel::V3D(0, 0, -10), Mantid::Kernel::V3D(0, 0, 0),
        Mantid::Kernel::V3D(1, 1, 1));

    auto inst2 = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);

    std::string path = "C:\\Users\\mqi61253\\testfile.txt"; // valid path

    TS_ASSERT_THROWS(saveInstrument(*inst2.first, path),
                     std::invalid_argument &);
  }
};

#endif /* MANTID_NEXUSGEOMETRY_NEXUSGEOMETRYSAVETEST_H_ */
