#ifndef MANTID_CUSTOMINTERFACES_STACKOFIMAGESDIRSTEST_H
#define MANTID_CUSTOMINTERFACES_STACKOFIMAGESDIRSTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Tomography/StackOfImagesDirs.h"

#include <boost/scoped_ptr.hpp>

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class StackOfImagesDirsTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StackOfImagesDirsTest *createSuite() {
    return new StackOfImagesDirsTest();
  }

  static void destroySuite(StackOfImagesDirsTest *suite) { delete suite; }

  StackOfImagesDirsTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure the framework is
                                               // initialized
  }

  void setUp() {
    // just to test more dynamic allocation
    m_soid.reset(new StackOfImagesDirs(""));
  }

  void tearDown() {}

  void test_construct() {
    StackOfImagesDirs obj("");

    TSM_ASSERT("A stack just constructed with an empty path string should not "
               "be valid",
               !obj.isValid());
  }

  void test_description() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("A description string should be produced",
               "" != obj.description());
  }

  void test_status() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("A status string should be produced", "" != obj.status());
  }

  void test_sampleImagesDir() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("The sample images directory of an empty stack should be empty",
               "" == obj.sampleImagesDir());
  }

  void test_flatImagesDir() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("The flat images directory of an empty stack should be empty",
               "" == obj.flatImagesDir());
  }

  void test_darkImagesDir() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("The dark images directory of an empty stack should be empty",
               "" == obj.flatImagesDir());
  }

  void test_sampleFiles() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("There should not be any sample files in an empty stack",
               0 == obj.sampleImagesDir().size());
  }

  void test_flatFiles() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("There should not be any flat image files in an empty stack",
               0 == obj.flatImagesDir().size());
  }

  void test_darkFiles() {
    StackOfImagesDirs obj("");

    TS_ASSERT_THROWS_NOTHING(obj.description());
    TSM_ASSERT("There should not be any dark image files in an empty stack",
               0 == obj.darkImagesDir().size());
  }

private:
  boost::scoped_ptr<StackOfImagesDirs> m_soid;
};

#endif /*  MANTID_CUSTOMINTERFACES_STACKOFIMAGESDIRSTEST_H */
