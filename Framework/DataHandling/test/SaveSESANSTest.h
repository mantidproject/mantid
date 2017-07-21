#ifndef MANTID_DATAHANDLING_SAVESESANSTEST_H_
#define MANTID_DATAHANDLING_SAVESESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveSESANS.h"
#include "MantidDataHandling/LoadSESANS.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "boost/algorithm/string/predicate.hpp"
#include <fstream>
#include <Poco/File.h>

using Mantid::DataHandling::SaveSESANS;

class SaveSESANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveSESANSTest *createSuite() { return new SaveSESANSTest(); }
  static void destroySuite( SaveSESANSTest *suite ) { delete suite; }

  void test_init() {
	  TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
	  TS_ASSERT(testAlg.isInitialized());
	  testAlg.setChild(true);
	  testAlg.setRethrows(true);
	  TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", outfileName));
	  testAlg.setProperty("Theta_zmax", 0.09);
	  testAlg.setProperty("Theta_ymax", 0.09);
  }

  void test_rejectTooManySpectra() {
	  auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
	  TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("InputWorkspace", ws));
	  
	  // Should throw, as we can't save more than one histogram
	  TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
  }

  void test_exec() {
	  // Set up workspace
	  auto ws = WorkspaceCreationHelper::create2DWorkspace(1, 10);
	  ws->setTitle("Sample workspace");
	  Mantid::API::Sample &sample = ws->mutableSample();
	  sample.setName("Sample sample");

	  testAlg.setProperty("InputWorkspace", ws);
	  
	  // Execute the algorithm
	  TS_ASSERT_THROWS_NOTHING(testAlg.execute());

	  // Get absolute path to the output file
	  std::string outputPath = testAlg.getPropertyValue("Filename");
	  std::string outws = "outws";

	  Mantid::DataHandling::LoadSESANS loader;
	  loader.initialize();
	  TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", outputPath));
	  TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", outws));
	  std::string result;
	  TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

private:
	SaveSESANS testAlg;
	const std::string outfileName = "temp.ses";
};


#endif /* MANTID_DATAHANDLING_SAVESESANSTEST_H_ */