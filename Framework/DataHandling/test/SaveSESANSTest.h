#ifndef MANTID_DATAHANDLING_SAVESESANSTEST_H_
#define MANTID_DATAHANDLING_SAVESESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveSESANS.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "boost/algorithm/string/predicate.hpp"
#include <fstream>
#include <iostream> // MAKE SURE THIS IS TAKEN OUT
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

	  TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
  }

  void test_writeHeaders() {
	  Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 10);
	  ws->setTitle("Sample workspace");
	  Mantid::API::Sample &sample = ws->mutableSample();
	  sample.setName("Sample sample");

	  Mantid::DataHandling::SaveSESANS testAlg;
	  testAlg.initialize();
	  testAlg.setChild(true);
	  testAlg.setRethrows(true);

	  testAlg.setProperty("InputWorkspace", ws);
	  testAlg.setProperty("Filename", "test.ses");
	  testAlg.setProperty("Theta_zmax", 0.09);
	  testAlg.setProperty("Theta_ymax", 0.09);

	  TS_ASSERT_THROWS_NOTHING(testAlg.execute());

	  std::string outputPath = testAlg.getPropertyValue("Filename");

	  TS_ASSERT(Poco::File(outputPath).exists());
	  std::ifstream file(outputPath);
	  TS_ASSERT(file.good());

	  std::string line;
	  file.clear();
	  std::cout << std::endl;
	  std::getline(file, line);

	  TS_ASSERT(boost::starts_with(line, "FileFormatVersion"));

	  bool beginFound = false;
	  while (!beginFound && std::getline(file, line))
		  if (line == "BEGIN_DATA")
			  beginFound = true;
	  
	  TS_ASSERT(beginFound);

	  file.close();
  }

};


#endif /* MANTID_DATAHANDLING_SAVESESANSTEST_H_ */