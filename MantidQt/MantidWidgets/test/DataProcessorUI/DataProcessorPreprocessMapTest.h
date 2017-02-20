#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSMAPTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSMAPTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessMap.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorPreprocessMapTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorPreprocessMapTest *createSuite() {
    return new DataProcessorPreprocessMapTest();
  }
  static void destroySuite(DataProcessorPreprocessMapTest *suite) {
    delete suite;
  }
  DataProcessorPreprocessMapTest() { FrameworkManager::Instance(); };

  void test_add_element() {
    DataProcessorPreprocessMap preprocessMap;
    preprocessMap.addElement("Runs", "Load");
    preprocessMap.addElement("Transmission Runs",
                             "CreateTransmissionWorkspaceAuto");

    auto preprocessingInstructions = preprocessMap.asMap();
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSMAPTEST_H */
