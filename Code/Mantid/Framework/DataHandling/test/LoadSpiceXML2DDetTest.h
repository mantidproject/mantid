#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpiceXML2DDet.h"

using Mantid::DataHandling::LoadSpiceXML2DDet;
using namespace Mantid::API;

class LoadSpiceXML2DDetTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSpiceXML2DDetTest *createSuite() {
    return new LoadSpiceXML2DDetTest();
  }
  static void destroySuite(LoadSpiceXML2DDetTest *suite) { delete suite; }

  void test_Init() {
    LoadSpiceXML2DDet testalg;
    testalg.initialize();
    TS_ASSERT(testalg.isInitialized());
  }

  void test_LoadHB3AXML() {
    LoadSpiceXML2DDet loader;
    loader.initialize();

    const std::string filename("HB3A_exp355_scan0001_0522.xml");
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "Exp0335_S0038"));

    loader.execute();
    TS_ASSERT(loader.isExecuted());
  }
};

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDETTEST_H_ */
