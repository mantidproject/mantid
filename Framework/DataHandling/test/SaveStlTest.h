// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVE_STL_TEST_H_
#define SAVE_STL_TEST_H_
#include "MantidDataHandling//SaveStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidKernel/V3D.h"

#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class SaveStlTest : public CxxTest::TestSuite {
public:
  static SaveStlTest *createSuite() { return new SaveStlTest(); }
  static void destroySuite(SaveStlTest *suite) { delete suite; }

  void test_saves_valid_stl() {
    std::string path =
        "/home/sjenkins/Documents/SampleCorrections/SaveStlTest.stl";
    std::vector<uint32_t> triangle{1, 0, 3, 1, 4, 0, 1, 3, 6, 3, 0, 7,
                                   3, 7, 6, 6, 0, 2, 4, 7, 0, 6, 7, 4,
                                   5, 2, 1, 1, 3, 5, 5, 4, 2, 6, 4, 5};
    std::vector<V3D> vertices{
        V3D(5, 5, -15), V3D(-5, -5, -15), V3D(-5, 5, -15), V3D(5, -5, -15),
        V3D(-5, 5, 15), V3D(-5, -5, 15),  V3D(5, -5, 15),  V3D(5, 5, 15)};
    auto writer = SaveStl(path, triangle, vertices, ScaleUnits::metres);
    writer.writeStl();
    auto reader = LoadBinaryStl(path, ScaleUnits::metres);
    TS_ASSERT(Poco::File(path).exists());
    TS_ASSERT(reader.isBinarySTL(path));
    Poco::File(path).remove();
  }

  void test_saves_shape_correctly() {
    std::string path =
        "/home/sjenkins/Documents/SampleCorrections/SaveStlTest.stl";
    std::vector<uint32_t> triangle{0, 1, 2, 0, 3, 1, 0, 2, 4, 2, 1, 5,
                                   2, 5, 4, 6, 1, 3, 6, 5, 1, 4, 5, 6,
                                   7, 3, 0, 0, 4, 7, 7, 6, 3, 4, 6, 7};
    std::vector<V3D> vertices{V3D(-5, -5, -15), V3D(5, 5, -15), V3D(5, -5, -15),
                              V3D(-5, 5, -15),  V3D(5, -5, 15), V3D(5, 5, 15),
                              V3D(-5, 5, 15),   V3D(-5, -5, 15)};
    std::vector<double> compareVertices{-5,  -5, -15, 5,   5,  -15, 5,  -5,
                                        -15, -5, 5,   -15, 5,  -5,  15, 5,
                                        5,   15, -5,  5,   15, -5,  -5, 15};
    auto writer = SaveStl(path, triangle, vertices, ScaleUnits::metres);
    writer.writeStl();
    auto reader = LoadBinaryStl("/home/sjenkins/Work/Build-1/ExternalData/"
                                "Testing/Data/UnitTest/cubeBin.stl",
                                ScaleUnits::metres);
    TS_ASSERT(Poco::File(path).exists());
    TS_ASSERT(reader.isBinarySTL(path));
    auto shape = reader.readStl();
    auto loadedTriangles = shape->getTriangles();
    auto loadedVertices = shape->getVertices();
    TS_ASSERT_EQUALS(loadedTriangles, triangle);
    TS_ASSERT_EQUALS(loadedVertices, compareVertices);
    Poco::File(path).remove();
  }

  void test_fails_invalid_shape(){
    std::string path =
        "/home/sjenkins/Documents/SampleCorrections/SaveStlTest.stl";
    std::vector<uint32_t> triangle{0, 1, 2, 0, 3, 1, 0, 2, 4, 2, 1, 5,
                                   2, 5, 4, 6, 1, 3, 6, 5, 1, 4, 5, 6,
                                   7, 3, 0, 0, 4, 7, 7, 6, 3, 4, };
    std::vector<V3D> vertices{V3D(-5, -5, -15), V3D(5, 5, -15), V3D(5, -5, -15),
                              V3D(-5, 5, -15),  V3D(5, -5, 15), V3D(5, 5, 15),
                              V3D(-5, 5, 15),   V3D(-5, -5, 15)};
    auto writer = SaveStl(path, triangle, vertices, ScaleUnits::metres);
    TS_ASSERT_THROWS(writer.writeStl(),std::runtime_error);
    TS_ASSERT(!Poco::File(path).exists());
  }
};
#endif /*SAVE_STL_TEST_H_*/