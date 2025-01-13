// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/SaveDaveGrp.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>

#include <fstream>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDaveGrpTest : public CxxTest::TestSuite {
public:
  static SaveDaveGrpTest *createSuite() { return new SaveDaveGrpTest(); }
  static void destroySuite(SaveDaveGrpTest *suite) { delete suite; }

  SaveDaveGrpTest() { saver = AlgorithmManager::Instance().create("SaveDaveGrp"); }

  ~SaveDaveGrpTest() override = default;

  void testName() { TS_ASSERT_EQUALS(saver->name(), "SaveDaveGrp"); }

  void testVersion() { TS_ASSERT_EQUALS(saver->version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(saver->initialize());
    TS_ASSERT(saver->isInitialized());

    TS_ASSERT_EQUALS(static_cast<int>(saver->getProperties().size()), 3);
  }

  void test_exec() {
    // Create a small test workspace
    std::string WSName = "saveDaveGrp_input";
    MatrixWorkspace_const_sptr input = makeWorkspace(WSName);

    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("InputWorkspace", WSName));
    std::string outputFile("testSaveDaveGrp1.grp");
    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("Filename", outputFile));
    outputFile = saver->getPropertyValue("Filename"); // get absolute path

    TS_ASSERT_THROWS_NOTHING(saver->execute());
    TS_ASSERT(saver->isExecuted());

    TS_ASSERT(Poco::File(outputFile).exists());

    // check the content of the file
    std::ifstream testfile;
    testfile.open(outputFile.c_str());
    TS_ASSERT(testfile.is_open());
    if (testfile.is_open()) {
      std::string line;
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Number of Energy transfer values");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "3");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Number of Y values");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "2");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Energy transfer (meV) values");
      double d1, d2;
      int i;
      for (i = 0; i <= 2; i++) {
        testfile >> d1;
        TS_ASSERT_EQUALS(d1, 1.5 + i);
      }
      getline(testfile, line);
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Y () values");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "1");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "2");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Group 0");
      for (i = 0; i < 3; i++) {
        testfile >> d1 >> d2;
        TS_ASSERT_EQUALS(d1, 2);
        TS_ASSERT_DELTA(d2, M_SQRT2, 0.0001);
      }
      getline(testfile, line);
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Group 1");
      for (i = 0; i < 3; i++) {
        testfile >> d1 >> d2;
        TS_ASSERT_EQUALS(d1, 2);
        TS_ASSERT_DELTA(d2, M_SQRT2, 0.0001);
      }
      testfile.close();
    }
    AnalysisDataService::Instance().remove(WSName);
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
  }

  void test_compare_to_original() {
    const std::string WSName("dave_grp");
    LoadDaveGrp loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "DaveAscii.grp"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("XAxisUnits", "DeltaE"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("YAxisUnits", "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty<bool>("IsMicroEV", true));
    std::string inputFile;
    inputFile = loader.getPropertyValue("Filename"); // get absolute path
    loader.execute();

    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("InputWorkspace", WSName));
    std::string outputFile("testSaveDaveGrp2.grp");
    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("Filename", outputFile));
    outputFile = saver->getPropertyValue("Filename"); // get absolute path
    TS_ASSERT_THROWS_NOTHING(saver->setProperty<bool>("ToMicroEV", true));
    TS_ASSERT_THROWS_NOTHING(saver->execute());
    TS_ASSERT(saver->isExecuted());

    TS_ASSERT(Poco::File(outputFile).exists());
    // check the content of the file
    std::ifstream testin, testout;
    testin.open(inputFile.c_str());
    testout.open(outputFile.c_str());
    TS_ASSERT(testin.is_open());
    TS_ASSERT(testout.is_open());
    if (testin.is_open() && testout.is_open()) {
      int i;
      std::string linein, lineout;
      for (i = 0; i <= 4; i++) {
        getline(testin, linein);
        boost::to_upper(linein);

        getline(testout, lineout);
        boost::to_upper(lineout);
        TS_ASSERT_EQUALS(linein.substr(0, 12), lineout.substr(0, 12)); // We have an extra "transfer"
                                                                       // in "Q transfer", and "(ueV)"
                                                                       // insted of "(micro eV)"
      }
      double din, dout;
      for (i = 0; i < 60; i++) {
        testin >> din;
        testout >> dout;
        TS_ASSERT_DELTA(din, dout, 1e-5);
      }
      getline(testin, linein);
      getline(testin, linein);
      boost::to_upper(linein);

      getline(testout, lineout);
      getline(testout, lineout);
      boost::to_upper(lineout);
      lineout.insert(4, "TRANSFER ");
      TS_ASSERT_EQUALS(linein.substr(0, 20), lineout.substr(0, 20));
      for (i = 0; i < 28; i++) {
        testin >> din;
        testout >> dout;
        TS_ASSERT_DELTA(din, dout, 1e-5);
      }
      getline(testin, linein);
      getline(testin, linein);
      getline(testout, lineout);
      getline(testout, lineout);
      TS_ASSERT_EQUALS(linein.substr(0, 20), lineout.substr(0, 20));
      for (i = 0; i < 60; i++) {
        testin >> din;
        testout >> dout;
        TS_ASSERT_DELTA(din, dout, 1e-7);
      }
      testin.close();
      testout.close();
    }
    AnalysisDataService::Instance().remove(WSName);
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
  }

  void test_exec_event() {
    LoadEventNexus ld;
    ld.initialize();
    std::string outws("CNCS");
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws);
    ld.setPropertyValue("Precount", "0");
    ld.setProperty("NumberOfBins", 1);
    ld.execute();
    TS_ASSERT(ld.isExecuted());
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS(dataStore.doesExist(outws), true);

    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("InputWorkspace", outws));
    std::string outputFile("testSaveDaveGrp3.grp");
    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("Filename", outputFile));
    outputFile = saver->getPropertyValue("Filename"); // get absolute path

    TS_ASSERT_THROWS_NOTHING(saver->execute());
    TS_ASSERT(saver->isExecuted());

    TS_ASSERT(Poco::File(outputFile).exists());
    // check the content of the file
    std::ifstream testfile;
    testfile.open(outputFile.c_str());
    TS_ASSERT(testfile.is_open());
    if (testfile.is_open()) {
      std::string line;
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Number of Time-of-flight values");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "1"); // only one bin
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Number of Y values");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "51200");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Time-of-flight (microsecond) values");
      double d;
      testfile >> d;
      TS_ASSERT_DELTA(d, 52496.4, 1);
      getline(testfile, line);
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Y () values");
      for (int i = 0; i < 51200; i++) {
        testfile >> d;
        TS_ASSERT_DELTA(d, static_cast<double>(i + 1), 0.001);
      }
      getline(testfile, line);
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Group 0");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "0 0");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "# Group 1");
      getline(testfile, line);
      TS_ASSERT_EQUALS(line, "0 0");
      testfile.close();
    }
    AnalysisDataService::Instance().remove(outws);
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
  }

private:
  IAlgorithm_sptr saver;

  MatrixWorkspace_sptr makeWorkspace(const std::string &input) {
    // all the Y values in this new workspace are set to DEFAU_Y, which
    // currently = 2
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 1.0);
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");
    AnalysisDataService::Instance().add(input, inputWS);
    return inputWS;
  }
};
