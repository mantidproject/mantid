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

#include "MantidDataHandling/SavePHX.h"
// to generate test workspaces
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/LoadInstrument.h"
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

class SavePHXTest : public CxxTest::TestSuite {
private:
  Mantid::DataHandling::SavePHX phxSaver;
  std::string TestOutputFile;
  std::string WSName;
  std::string TestOutputParTableWSName;

public:
  static SavePHXTest *createSuite() { return new SavePHXTest(); }
  static void destroySuite(SavePHXTest *suite) { delete suite; }

  void testAlgorithmName() { TS_ASSERT_EQUALS(phxSaver.name(), "SavePHX"); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(phxSaver.initialize());
    TS_ASSERT(phxSaver.isInitialized());
  }
  void testExec() {
    // Create a small test workspace
    WSName = "savePHXTest_input";
    API::MatrixWorkspace_const_sptr input = makeWorkspace(WSName);

    TS_ASSERT_THROWS_NOTHING(phxSaver.setPropertyValue("InputWorkspace", WSName));
    TestOutputFile = std::string("testPHX.phx");
    TS_ASSERT_THROWS_NOTHING(phxSaver.setPropertyValue("Filename", TestOutputFile));
    TestOutputFile = phxSaver.getPropertyValue("Filename"); // get absolute path

    this->TestOutputParTableWSName = "TestOutputParWS";
    phxSaver.set_resulting_workspace(TestOutputParTableWSName);

    TS_ASSERT_THROWS_NOTHING(phxSaver.execute());
    TS_ASSERT(phxSaver.isExecuted());
  }

  /* void ttResutlts(){
       std::vector<std::string> pattern(5),count(5);
       std::string result;
       pattern[0]=" 3";
       pattern[1]=" 1.000	 0 		170.565 	0.000 	0.792
   5.725
   1";
       pattern[2]=" 1.000	 0 		169.565 	0.000 	0.790
   5.725
   2";
       pattern[3]=" 1.000	 0 		168.565 	0.000 	0.787
   5.725
   3";
       count[0]=" 0 ";count[1]=" 1 ";count[2]=" 2 ";count[3]=" 3 ";

       std::ifstream testFile;
       testFile.open(TestOutputFile.c_str());
       TSM_ASSERT(" Can not open test file produced by algorithm
   phxSaver",testFile.is_open());
       int ic(0);
       while(ic<5){
           std::getline(testFile,result);
           if(testFile.eof())break;

           TSM_ASSERT_EQUALS("wrong string N "+count[ic]+" obtained from
   file",pattern[ic],result);
           ic++;
       }
       TSM_ASSERT_EQUALS(" Expecting 4 rows ascii file, but got different number
   of rows",4,ic);
       testFile.close();

   }*/
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
    column_name[3] = "polar_width";
    column_name[4] = "azimuthal_width";
    column_name[5] = "detID";

    API::Workspace_sptr sample = API::AnalysisDataService::Instance().retrieve(TestOutputParTableWSName);
    DataObjects::TableWorkspace_sptr spTW = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(sample);
    TSM_ASSERT("should be able to retrieve sample workspace from the dataservice", spTW);

    std::ifstream testFile;
    testFile.open(TestOutputFile.c_str());
    TSM_ASSERT(" Can not open test file produced by algorithm PHXSaver", testFile.is_open());
    int ic(0);
    std::vector<float> sample_value(7, 0);
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
        size_t ii = 0;
        for (const auto &i : column_name) {
          sample_value[ii] = (spTW->cell_cast<float>(ic - 1, i));
          ii++;
          if (ii == 1)
            ii = 2; // scip second column in the file, which contains 0;
        }
      }

      for (size_t i = 0; i < test.size(); i++) {
        TSM_ASSERT_DELTA("wrong sring: " + count[ic] + "  obtained from file;", sample_value[i], test[i], 1e-3);
      }
      ic++;
    }
    TSM_ASSERT_EQUALS(" Expecting 4 rows ascii file, but got different number of rows", 4, ic);
    testFile.close();
  }

private:
  ~SavePHXTest() override {
    // delete test ws from ds after the test ends
    AnalysisDataService::Instance().remove(WSName);
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
    loader.setPropertyValue("Workspace", input);
    loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loader.execute();

    // required to get it passed the algorthms validator
    inputWS->setDistribution(true);

    return inputWS;
  }
};
