// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <fstream>

#include "MantidDataHandling/SavePAR.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
// to generate test workspaces
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Geometry::Instrument;

static const int NHIST = 3;
static const int THEMASKED = 2;

class SavePARTest : public CxxTest::TestSuite {
private:
  Mantid::DataHandling::SavePAR parSaver;
  std::string TestOutputFile;
  std::string WSName;
  std::string TestOutputParTableWSName;

public:
  static SavePARTest *createSuite() { return new SavePARTest(); }
  static void destroySuite(SavePARTest *suite) { delete suite; }

  void testAlgorithmName() { TS_ASSERT_EQUALS(parSaver.name(), "SavePAR"); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(parSaver.initialize());
    TS_ASSERT(parSaver.isInitialized());
  }
  void testExec() {
    // Create a small test workspace
    WSName = "savePARTest_input";
    API::MatrixWorkspace_const_sptr input = makeWorkspace(WSName);

    TS_ASSERT_THROWS_NOTHING(parSaver.setPropertyValue("InputWorkspace", WSName));
    TestOutputFile = std::string("testPAR.par");
    TS_ASSERT_THROWS_NOTHING(parSaver.setPropertyValue("Filename", TestOutputFile));
    TestOutputFile = parSaver.getPropertyValue("Filename"); // get absolute path

    // set resulting test par workspace to compare results against
    this->TestOutputParTableWSName = "TestOutputParWS";
    parSaver.set_resulting_workspace(TestOutputParTableWSName);

    // exec algorithm
    TS_ASSERT_THROWS_NOTHING(parSaver.execute());
    TS_ASSERT(parSaver.isExecuted());
  }

  void testResutlts() {
    std::vector<std::string> count(5), column_name(6);
    std::string result;
    count[0] = " 0 ";
    count[1] = " 1 ";
    count[2] = " 2 ";
    count[3] = " 3 ";

    // this is column names as they are defined in table workspace, but placed
    // in the positions of the file
    column_name[0] = "secondary_flightpath";
    column_name[1] = "twoTheta";
    column_name[2] = "azimuthal";
    column_name[3] = "det_width";
    column_name[4] = "det_height";
    column_name[5] = "detID";

    API::Workspace_sptr sample = API::AnalysisDataService::Instance().retrieve(TestOutputParTableWSName);
    DataObjects::TableWorkspace_sptr spTW = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(sample);
    TSM_ASSERT("should be able to retrieve sample workspace from the dataservice", spTW);

    std::ifstream testFile;
    testFile.open(TestOutputFile.c_str());
    TSM_ASSERT(" Can not open test file produced by algorithm PARSaver", testFile.is_open());
    int ic(0);
    std::vector<float> sample_value(6);
    while (ic < 5) {
      // get data from file
      std::getline(testFile, result);
      if (testFile.eof())
        break;
      std::vector<float> test = Kernel::VectorHelper::splitStringIntoVector<float>(result);

      // get sample value[s];
      if (ic == 0) {
        sample_value[0] = (float)spTW->rowCount();
      } else {
        for (size_t i = 0; i < test.size(); i++) {
          sample_value[i] = (spTW->cell_cast<float>(ic - 1, column_name[i]));
        }
      }

      for (size_t i = 0; i < test.size(); i++) {
        TSM_ASSERT_DELTA("wrong sring: " + count[ic] + " column: " + column_name[i] + " obtained from file;",
                         sample_value[i], test[i], 1e-3);
      }
      ic++;
    }
    TSM_ASSERT_EQUALS(" Expecting 4 rows ascii file, but got different number of rows", 4, ic);
    testFile.close();
  }

private:
  ~SavePARTest() override {
    // delete test ws from ds after the test ends
    API::AnalysisDataService::Instance().remove(WSName);
    API::AnalysisDataService::Instance().remove(TestOutputParTableWSName);
    // delete test output file from the hdd;
    std::filesystem::remove(TestOutputFile);
  }

  MatrixWorkspace_sptr makeWorkspace(const std::string &input) {
    // all the Y values in this new workspace are set to DEFAU_Y, which
    // currently = 2
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(NHIST, 10, 1.0);
    return setUpWorkspace(input, inputWS);
  }

  MatrixWorkspace_sptr setUpWorkspace(const std::string &input, MatrixWorkspace_sptr inputWS) {
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");

    // we do not need to deal with analysisi data service here in test to avoid
    // holding the workspace there after the test
    AnalysisDataService::Instance().add(input, inputWS);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loader.setPropertyValue("Workspace", input);
    loader.execute();

    // required to get it passed the algorthms validator
    inputWS->setDistribution(true);

    return inputWS;
  }
};
