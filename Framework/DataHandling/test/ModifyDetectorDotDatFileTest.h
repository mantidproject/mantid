// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/ModifyDetectorDotDatFile.h"
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class ModifyDetectorDotDatFileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModifyDetectorDotDatFileTest *createSuite() { return new ModifyDetectorDotDatFileTest(); }
  static void destroySuite(ModifyDetectorDotDatFileTest *suite) { delete suite; }

  // Helper to set up a simple workspace for testing
  void makeTestWorkspace(const std::string &ads_name) {
    IAlgorithm *loader;
    loader = new Mantid::DataHandling::LoadEmptyInstrument;
    loader->initialize();
    TS_ASSERT_THROWS_NOTHING(loader->setPropertyValue("Filename", "unit_testing/MAPS_Definition_Reduced.xml"));
    loader->setPropertyValue("OutputWorkspace", ads_name);
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
    delete loader;
  }

  void test_Init() {
    ModifyDetectorDotDatFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    ModifyDetectorDotDatFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    // Create input workspace
    std::string wsName = "ModifyDetectorDotDatFileTestWorkspace";
    makeTestWorkspace(wsName);

    // Test Properties
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputFilename", "detector_few_maps.dat"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputFilename", "detector_few_maps_result.dat"));

    // Test execution
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check output file
    std::string fullFilename = alg.getPropertyValue("OutputFilename"); // Get absolute path
    // has the algorithm written the output file to disk?
    bool OutputFileExists = Poco::File(fullFilename).exists();
    TS_ASSERT(OutputFileExists);
    // If output file exists do some tests on its contents
    if (OutputFileExists) {
      std::ifstream in(fullFilename.c_str());
      std::string header;
      std::string ignore;
      std::string columnNames;
      std::string detectorData;
      int detNo;
      double offset;
      double l2;
      int code;
      double theta;
      double phi;

      // Check header has name of algorithm in it
      getline(in, header);
      bool headerHasNameOfAlgorithmInIt = header.find("ModifyDetectorDotDatFile") != std::string::npos;
      TS_ASSERT(headerHasNameOfAlgorithmInIt);

      // Ignore 2nd line
      TS_ASSERT_THROWS_NOTHING(getline(in, ignore));

      // Now at 3rd line
      TS_ASSERT_THROWS_NOTHING(getline(in, columnNames));
      TS_ASSERT_EQUALS(columnNames.substr(0, 9), "  det no.");

      // Look for detector 11208002 and verify there are at least 7 detectors
      bool detector11208002found = false;
      for (int i = 1; i < 8; ++i) {
        TS_ASSERT_THROWS_NOTHING(getline(in, detectorData));
        if (detectorData.substr(0, 9) == " 11208002") {
          detector11208002found = true;
          std::istringstream is(detectorData);
          TS_ASSERT_THROWS_NOTHING(is >> detNo >> offset >> l2 >> code >> theta >> phi);
        }
      }

      TS_ASSERT(detector11208002found);

      // If the detector has been found, test some of its data
      if (detector11208002found) {
        TS_ASSERT_EQUALS(code, 3);           // Not changed by algorithm
        TS_ASSERT_DELTA(offset, 5.3, 0.001); // Not changed by algorithm
        TS_ASSERT_DELTA(l2, 6.02008,
                        0.00001); // Changed from 3.02008 by algorithm
        TS_ASSERT_DELTA(theta, 8.36362,
                        0.00001); // Changed from 4.36362 by algorithm
        TS_ASSERT_DELTA(phi, 34.12505,
                        0.00001); // Changed from 17.12505 by algorithm
      }

      in.close();
    }

    // Remove output file
    Poco::File(fullFilename).remove();
  }
};
