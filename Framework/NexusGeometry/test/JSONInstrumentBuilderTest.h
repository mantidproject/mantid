// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDERTEST_H_
#define MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexusGeometry/JSONInstrumentBuilder.h"

using Mantid::NexusGeometry::JSONInstrumentBuilder;

class JSONInstrumentBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JSONInstrumentBuilderTest *createSuite() { return new JSONInstrumentBuilderTest(); }
  static void destroySuite( JSONInstrumentBuilderTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDERTEST_H_ */