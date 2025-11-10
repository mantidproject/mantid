// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/FFTSmooth2.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms::FFTSmooth::PropertyNames;

class FFTSmooth2Test : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(fftsmooth2.name(), "FFTSmooth") }

  void testVersion() { TS_ASSERT_EQUALS(fftsmooth2.version(), 2) }

  void testInit() {
    Mantid::Algorithms::FFTSmooth::FFTSmooth2 fftsmooth2_b;
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_b.initialize());
    TS_ASSERT(fftsmooth2_b.isInitialized());

    const std::vector<Property *> props = fftsmooth2_b.getProperties();
    TS_ASSERT_EQUALS(props.size(), 7);

    TS_ASSERT_EQUALS(props[0]->name(), INPUT_WKSP);
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), OUTPUT_WKSP);
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), WKSP_INDEX);
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<int> *>(props[2]));

    TS_ASSERT_EQUALS(props[3]->name(), FILTER);
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT_EQUALS(props[3]->value(), "Zeroing");
    // NOTE: enumerated string properties cannot be converted to a PropertyWithValue
    // To test here, the enum and vector would need to be accessible, but they are not

    TS_ASSERT_EQUALS(props[4]->name(), PARAMS);
    TS_ASSERT(props[4]->isDefault());
    TS_ASSERT(!props[4]->value().empty()); // will equal default of {2, 2}
    TS_ASSERT(dynamic_cast<ArrayProperty<std::size_t> *>(props[4]));
  }

  // check the Params property can be set with strings
  void doTestSetParams(std::string const &input, std::vector<std::size_t> const &res) {
    Mantid::Algorithms::FFTSmooth::FFTSmooth2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(PARAMS, input));

    std::vector<std::size_t> params = alg.getProperty(PARAMS);
    TS_ASSERT(!params.empty());
    TS_ASSERT_EQUALS(params, res);
  }

  void testSetParams() { // set the parameter values every possible way
    Mantid::Algorithms::FFTSmooth::FFTSmooth2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // set one value
    doTestSetParams(" 12\t ", {12});

    // set with space
    doTestSetParams(" 7  3 ", {7, 3});

    // set with comma
    doTestSetParams("3, 4  ", {3, 4});

    // set with semicolon
    doTestSetParams(" 5; 6", {5, 6});

    // set with colon
    doTestSetParams("7:8 ", {7, 8});

    // set with tab
    doTestSetParams("9\t10", {9, 10});
  }

  void testZeroing() { // load input and "Gold" result workspaces
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename", "MultispectralTestData.nxs");
    loader.setProperty("OutputWorkspace", "TestInputWS");
    loader.execute();

    loader.initialize();
    loader.setProperty("Filename", "FFTSmooth2_Zeroing.nxs");
    loader.setProperty("OutputWorkspace", "ZeroingGoldWS");
    loader.execute();
    // create and execute the algorithm for "Zeroing"
    Mantid::Algorithms::FFTSmooth::FFTSmooth2 fftsmooth2_c;
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.initialize());
    TS_ASSERT(fftsmooth2_c.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(INPUT_WKSP, "TestInputWS"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(OUTPUT_WKSP, "SmoothedWS"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(WKSP_INDEX, "0"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(FILTER, "Zeroing"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(PARAMS, "100"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.execute());
    TS_ASSERT(fftsmooth2_c.isExecuted());

    MatrixWorkspace_sptr test_output_WS;
    MatrixWorkspace_sptr gold_output_WS;

    TS_ASSERT_THROWS_NOTHING(test_output_WS =
                                 AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("SmoothedWS"));
    TS_ASSERT(test_output_WS);

    TS_ASSERT_THROWS_NOTHING(gold_output_WS =
                                 AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ZeroingGoldWS"));
    TS_ASSERT(gold_output_WS);

    TS_ASSERT_EQUALS(test_output_WS->size(), gold_output_WS->size());

    for (size_t i = 0; i < test_output_WS->size(); i++) {
      TS_ASSERT_DELTA(test_output_WS->y(0)[i], gold_output_WS->y(0)[i], 0.00001);
    }

    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");

    //  std::cout<< "RUNNING FFTSmooth2 testZeroing() DONE!\n";
  }

  void testButterworth() { // load input and "Gold" result workspaces
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename", "MultispectralTestData.nxs");
    loader.setProperty("OutputWorkspace", "TestInputWS");
    loader.execute();

    loader.initialize();
    loader.setProperty("Filename", "FFTSmooth2_Butterworth.nxs");
    loader.setProperty("OutputWorkspace", "ButterworthGoldWS");
    loader.execute();
    // create and execute the algorithm for "Butterworth"
    Mantid::Algorithms::FFTSmooth::FFTSmooth2 fftsmooth2_c;
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.initialize());
    TS_ASSERT(fftsmooth2_c.isInitialized());

    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(INPUT_WKSP, "TestInputWS"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(OUTPUT_WKSP, "SmoothedWS"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(WKSP_INDEX, "0"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(FILTER, "Butterworth"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.setPropertyValue(PARAMS, "100,2"));
    TS_ASSERT_THROWS_NOTHING(fftsmooth2_c.execute());
    TS_ASSERT(fftsmooth2_c.isExecuted());

    MatrixWorkspace_sptr test_output_WS;
    MatrixWorkspace_sptr gold_output_WS;

    TS_ASSERT_THROWS_NOTHING(test_output_WS =
                                 AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("SmoothedWS"));
    TS_ASSERT(test_output_WS);

    TS_ASSERT_THROWS_NOTHING(gold_output_WS =
                                 AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ButterworthGoldWS"));
    TS_ASSERT(gold_output_WS);

    TS_ASSERT_EQUALS(test_output_WS->size(), gold_output_WS->size());

    for (size_t i = 0; i < test_output_WS->size(); i++) {
      TS_ASSERT_DELTA(test_output_WS->y(0)[i], gold_output_WS->y(0)[i], 0.00001);
    }

    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");

    //  std::cout<< "RUNNING FFTSmooth2 testZeroing() DONE!\n";
  }

  //-------------------------------------------------------------------------------------------------
  void performTest(bool event, const std::string &filter, const std::string &params, bool AllSpectra,
                   int WorkspaceIndex, bool inPlace = false) {
    MatrixWorkspace_sptr ws1, out;
    int numPixels = 10;
    int numBins = 20;

    // Make workspaces where Y value == workspace index
    if (event)
      ws1 = WorkspaceCreationHelper::createEventWorkspace(10, numBins, numBins, 0, 1.0, 4);
    else
      ws1 = WorkspaceCreationHelper::create2DWorkspaceWhereYIsWorkspaceIndex(numPixels, numBins);

    std::string outName = "SmoothedWS";

    Mantid::Algorithms::FFTSmooth::FFTSmooth2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    if (inPlace) {
      AnalysisDataService::Instance().addOrReplace("FFTSmooth2WsInput", ws1);
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(INPUT_WKSP, "FFTSmooth2WsInput"));
      outName = "FFTSmooth2WsInput";
    } else {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(INPUT_WKSP, ws1));
    }

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(OUTPUT_WKSP, outName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(WKSP_INDEX, WorkspaceIndex));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(FILTER, filter));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(PARAMS, params));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(ALL_SPECTRA, AllSpectra));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outName));
    TS_ASSERT(out);
    if (!out)
      return;

    TS_ASSERT_EQUALS(out->blocksize(), numBins);
    if (AllSpectra) {
      TS_ASSERT_EQUALS(out->getNumberHistograms(), numPixels);
    } else {
      TS_ASSERT_EQUALS(out->getNumberHistograms(), 1);
    }

    for (size_t wi = 0; wi < out->getNumberHistograms(); wi++) {
      const auto &y = out->y(wi);
      double value = static_cast<double>(WorkspaceIndex);
      if (AllSpectra)
        value = static_cast<double>(wi);

      for (size_t j = 0; j < y.size(); ++j) {
        // Because the spectra are flat, the smoothing won't do much
        TS_ASSERT_DELTA(y[j], value, 0.02);
      }
    }
  }

  //-------------------------------------------------------------------------------------------------
  // Some specific tests
  void test_Event_Butterworth_AllSpectra() { performTest(true, "Butterworth", "100,2", true, 1); }

  void test_2D_Butterworth_AllSpectra() { performTest(false, "Butterworth", "100,2", true, 2); }

  void test_Event_Zeroing_AllSpectra() { performTest(true, "Zeroing", "100", true, 3); }

  void test_2D_Zeroing_AllSpectra() { performTest(false, "Zeroing", "100", true, 4); }

  void test_Event_Butterworth_SingleSpectrum() { performTest(true, "Butterworth", "100,2", false, 4); }

  void test_Event_Zeroing_SingleSpectrum() { performTest(true, "Zeroing", "100", false, 6); }

  //-------------------------------------------------------------------------------------------------
  /** Complete test of all possible inputs!
   * A total of 88 combinations are tested...
   * */
  void test_Everything() {
    for (int event = 0; event < 2; event++)
      for (int filterNum = 0; filterNum < 2; filterNum++)
        for (int AllSpectra = 0; AllSpectra < 2; AllSpectra++)
          for (int inPlace = 0; inPlace < 2; inPlace++) {
            std::string filter = "Zeroing";
            std::string params = "100";
            if (filterNum == 1) {
              filter = "Butterworth";
              params = "100,2";
            }

            if (!AllSpectra) {
              bool allSpectraGtZero = false;

              for (int WorkspaceIndex = 0; WorkspaceIndex < 10; WorkspaceIndex++) {
                performTest((event > 0), filter, params, allSpectraGtZero, WorkspaceIndex, (inPlace > 0));
              }
            } else {
              performTest((event > 0), filter, params, (AllSpectra > 0), 0, (inPlace > 0));
            }
          }
  }

private:
  Mantid::Algorithms::FFTSmooth::FFTSmooth2 fftsmooth2;
};
