#ifndef MANTID_DATAOBJECTS_PEAKSHAPENONETEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPENONETEST_H_

#ifdef _WIN32
#pragma warning(disable : 4251)
#endif

#include <cxxtest/TestSuite.h>
#include <json/json.h>
#include "MantidDataObjects/NoShape.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

using namespace Mantid::Kernel;
using Mantid::DataObjects::NoShape;

class NoShapeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NoShapeTest *createSuite() { return new NoShapeTest(); }
  static void destroySuite(NoShapeTest *suite) { delete suite; }

  void test_constructor() {

    // Construct it.
    NoShape shape;


    TS_ASSERT_EQUALS(Mantid::Kernel::None, shape.frame());
    TS_ASSERT_EQUALS(std::string(), shape.algorithmName());
    TS_ASSERT_EQUALS(-1, shape.algorithmVersion());
  }


  void test_toJSON() {

    // Construct it.
    NoShape shape;;
    const std::string json = shape.toJSON();

    Json::Reader reader;
    Json::Value output;
    TSM_ASSERT("Should parse as JSON", reader.parse(json, output));

    TS_ASSERT_EQUALS("none", output["shape"].asString());
  }


  void test_shape_name() {

    // Construct it.
    NoShape shape;

    TS_ASSERT_EQUALS("none", shape.shapeName());
  }
};

#endif /* MANTID_DATAOBJECTS_PEAKSHAPENONETEST_H_ */
