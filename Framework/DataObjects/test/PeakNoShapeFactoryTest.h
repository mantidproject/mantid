// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakNoShapeFactory.h"

using namespace Mantid::DataObjects;

class PeakNoShapeFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakNoShapeFactoryTest *createSuite() { return new PeakNoShapeFactoryTest(); }
  static void destroySuite(PeakNoShapeFactoryTest *suite) { delete suite; }

  void test_create() {
    PeakNoShapeFactory factory;
    Mantid::Geometry::PeakShape *product = factory.create("-**-");
    TS_ASSERT(dynamic_cast<NoShape *>(product));
    TS_ASSERT_EQUALS("none", product->shapeName());
    delete product;
  }
};
