#ifndef MANTID_DATAHANDLING_LOADSESANSTEST_H_
#define MANTID_DATAHANDLING_LOADSESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadSESANS.h"
#include "MantidKernel/FileDescriptor.h"

#include <Poco/File.h>
#include <Poco/TemporaryFile.h>

using Mantid::DataHandling::LoadSESANS;

class LoadSESANSTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSESANSTest *createSuite() { return new LoadSESANSTest(); }
  static void destroySuite(LoadSESANSTest *suite) { delete suite; }

  void test_init() {
	TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);

	TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", 
		getTestFilePath("LoadSESANSTest_goodFile.ses")));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputWorkspace", "ws"));
  }

  void test_exec() {
	  TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename",
		  getTestFilePath("LoadSESANSTest_goodFile.ses")));
    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    Mantid::API::MatrixWorkspace_sptr ws =
        testAlg.getProperty("OutputWorkspace");
    Mantid::API::Sample sample = ws->sample();

    // Make sure output properties were set correctly
    TS_ASSERT_EQUALS(ws->getTitle(), "PMMA in Mixed Deuterated decalin");
    TS_ASSERT_EQUALS(sample.getName(), "Ostensibly 40$ 100nm radius PMMA hard "
                                       "spheres in mixed deuterarted decalin.");
    TS_ASSERT_EQUALS(sample.getThickness(), 2.0);

    // Make sure the spectrum was written correctly
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    // One line should have been dropped, as it did not have enough columns
    TS_ASSERT_EQUALS(ws->getNPoints(), 5);

    double tolerance = 1e-5;
    // Test the first two rows we read
    // These values are all hard-coded in the sample file in writeGoodFile(),
    // using:
    // Y = e ^ (depol * wavelength ^2)
    // E = depolError * Y * wavelength ^ 2
    // X = wavelength
    TS_ASSERT_DELTA(ws->x(0)[0], 1.612452, tolerance);
    TS_ASSERT_DELTA(ws->y(0)[0], exp(-1.42e-3 * 1.612452 * 1.612452),
                    tolerance);
    TS_ASSERT_DELTA(ws->e(0)[0], 2.04e-3 * ws->y(0)[0] * 1.612452 * 1.612452,
                    tolerance);

    TS_ASSERT_DELTA(ws->x(0)[1], 1.675709, tolerance);
    TS_ASSERT_DELTA(ws->y(0)[1], exp(-1.45e-3 * 1.675709 * 1.675709),
                    tolerance);
    TS_ASSERT_DELTA(ws->e(0)[1], 1.87e-3 * ws->y(0)[0] * 1.675709 * 1.675709,
                    tolerance);
  }

  void test_confidence() {
    // Cannot use Poco::TemporaryFile, as we need to specify the file extension
	  TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename",
		  getTestFilePath("LoadSESANSTest_goodFile.ses")));

	  Mantid::Kernel::FileDescriptor descriptor(testAlg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(testAlg.confidence(descriptor), 70);
  }

  void test_requireFFV() { attemptToLoadBadFile("LoadSESANSTest_missingFFV.ses"); }

  void test_mandatoryHeaders() { attemptToLoadBadFile("LoadSESANSTest_missingHeaders.ses"); }

  void test_mandatoryColumns() { attemptToLoadBadFile("LoadSESANSTest_missingColumns.ses"); }

private:
  std::string getTestFilePath(const std::string &filename) {
		const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(filename);
		TS_ASSERT_DIFFERS(filepath, "");
		return filepath;
  }

  /// Try and fail to load a file which violates the allowed format
  void attemptToLoadBadFile(const std::string &filename) {
	  const std::string filepath = getTestFilePath(filename);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", filepath));
    TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
  }

  std::string createTemporaryFile() {
    Poco::TemporaryFile tempFile(".");
    tempFile.keepUntilExit();
    return tempFile.path();
  }

  Poco::File writeFile(std::string fileContents, std::string filepath) {
    // Write a file to our temporary file
	Poco::File pFile(filepath);
    std::ofstream file(pFile.path());
	if (file.fail()) {
		const std::string errorString = strerror(errno);
		throw new std::runtime_error("Failed to open file " + filepath + 
			" in LoadSESANSTest.\n The error returned was " + errorString);
	}
    file << fileContents;
    file.close();
	if (file.fail()) {
		const std::string errorString = strerror(errno);
		throw new std::runtime_error("Failed to close file " + filepath +
			" in LoadSESANSTest.\n The error returned was " + errorString);
	}
	return pFile;
  }

  LoadSESANS testAlg;

};

#endif /* MANTID_DATAHANDLING_LOADSESANSTEST_H_ */
