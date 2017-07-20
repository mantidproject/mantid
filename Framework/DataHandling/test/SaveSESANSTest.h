#ifndef MANTID_DATAHANDLING_SAVESESANSTEST_H_
#define MANTID_DATAHANDLING_SAVESESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveSESANS.h"
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

  void test_rejectTooManySpectra() {
	  auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
	  Mantid::DataHandling::SaveSESANS testAlg;
	  testAlg.initialize();
	  testAlg.setChild(true);
	  testAlg.setRethrows(true);
	  testAlg.setProperty("InputWorkspace", ws);
	  testAlg.setProperty("Filename", "test.ses");

	  // Should throw, as we can't save more than one histogram
	  TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
  }

  void test_writeHeaders() {
	  // Set up workspace
	  Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 10);
	  ws->setTitle("Sample workspace");
	  Mantid::API::Sample &sample = ws->mutableSample();
	  sample.setName("Sample sample");

	  // Set up algorithm
	  Mantid::DataHandling::SaveSESANS testAlg;
	  testAlg.initialize();
	  testAlg.setChild(true);
	  testAlg.setRethrows(true);
	  testAlg.setProperty("InputWorkspace", ws);
	  testAlg.setProperty("Filename", "test.ses");
	  testAlg.setProperty("Theta_zmax", 0.09);
	  testAlg.setProperty("Theta_ymax", 0.09);

	  // Execute the algorithm
	  TS_ASSERT_THROWS_NOTHING(testAlg.execute());

	  // Get absolute path to the output file
	  std::string outputPath = testAlg.getPropertyValue("Filename");

	  // Make sure it exists, and we can read it
	  TS_ASSERT(Poco::File(outputPath).exists());
	  std::ifstream file(outputPath);
	  TS_ASSERT(file.good());

	  std::string line;
	  std::getline(file, line);

	  // First line should give the File Format Version
	  TS_ASSERT(boost::starts_with(line, "FileFormatVersion"));

	  // BEGIN_DATA must come after the headers
	  bool beginFound = false;
	  while (!beginFound && std::getline(file, line))
		  if (line == "BEGIN_DATA")
			  beginFound = true;
	  
	  TS_ASSERT(beginFound);

	  file.close();
  }

};


#endif /* MANTID_DATAHANDLING_SAVESESANSTEST_H_ */