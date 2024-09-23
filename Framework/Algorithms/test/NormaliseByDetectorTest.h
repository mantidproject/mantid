// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Timer.h"
#include <boost/format.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/NormaliseByDetector.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using ScopedFileHelper::ScopedFile;

/**
Helper function. Runs LoadParameterAlg, to get an instrument parameter
definition from a file onto a workspace.
*/
void apply_instrument_parameter_file_to_workspace(const MatrixWorkspace_sptr &ws, const ScopedFile &file) {
  // Load the Instrument Parameter file over the existing test workspace +
  // instrument.
  using DataHandling::LoadParameterFile;
  LoadParameterFile loadParameterAlg;
  loadParameterAlg.setRethrows(true);
  loadParameterAlg.initialize();
  loadParameterAlg.setPropertyValue("Filename", file.getFileName());
  loadParameterAlg.setProperty("Workspace", ws);
  loadParameterAlg.execute();
}

/**
Helper method for running the algorithm and simply verifying that it runs
without exception producing an output workspace..
*/
MatrixWorkspace_sptr do_test_doesnt_throw_on_execution(const MatrixWorkspace_sptr &inputWS, bool parallel = true) {
  // Needs other algorithms and functions to be registered
  FrameworkManager::Instance();
  NormaliseByDetector alg(parallel);
  alg.setRethrows(true);
  alg.initialize();
  alg.setPropertyValue("OutputWorkspace", "out");
  alg.setProperty("InputWorkspace", inputWS);
  TS_ASSERT_THROWS_NOTHING(alg.execute());
  MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
  TS_ASSERT(outWS != nullptr);
  return outWS;
}

/**

Functionality Tests

*/
class NormaliseByDetectorTest : public CxxTest::TestSuite {

private:
  /** Helper function, creates a histogram workspace with an instrument with 2
     detectors, and 2 spectra.
      Y-values are flat accross the x bins. Which makes it easy to calculate the
     expected value for any fit function applied to the X-data.
  */
  MatrixWorkspace_sptr create_workspace_with_no_fitting_functions() {
    const std::string outWSName = "test_ws";
    auto workspaceAlg = AlgorithmManager::Instance().create("CreateWorkspace");
    workspaceAlg->initialize();
    workspaceAlg->setPropertyValue("DataX", "1, 2, 3, 4");       // 4 bins.
    workspaceAlg->setPropertyValue("DataY", "1, 1, 1, 1, 1, 1"); // Each spectrum gets 3 Y values
    workspaceAlg->setPropertyValue("DataE", "1, 1, 1, 1, 1, 1"); // Each spectrum gets 3 E values
    workspaceAlg->setPropertyValue("NSpec", "2");
    workspaceAlg->setPropertyValue("UnitX", "Wavelength");
    workspaceAlg->setPropertyValue("OutputWorkspace", outWSName);
    workspaceAlg->execute();
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    ws->setInstrument(ComponentCreationHelper::createTestInstrumentRectangular(6, 1, 0));
    ws->getSpectrum(0).setDetectorID(1);
    ws->getSpectrum(1).setDetectorID(2);
    return ws;
  }

  /**
   Helper function, applies fit functions from a fabricated, fake instrument
   parameter file ontop of an existing instrument definition.
   The fit function is set at the instrument level.
  */
  MatrixWorkspace_sptr create_workspace_with_fitting_functions(const std::string &result_unit = "Wavelength") {
    // Create a default workspace with no-fitting functions.
    MatrixWorkspace_sptr ws = create_workspace_with_no_fitting_functions();
    const std::string instrumentName = ws->getInstrument()->getName();

    // Create a parameter file, with a root equation that will apply to all
    // detectors.
    const std::string parameterFileContents = boost::str(boost::format("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
       <parameter-file instrument = \"%1%\" date = \"2012-01-31T00:00:00\">\n\
          <component-link name=\"%1%\">\n\
           <parameter name=\"LinearBackground:A0\" type=\"fitting\">\n\
               <formula eq=\"1\" result-unit=\"%2%\"/>\n\
               <fixed />\n\
           </parameter>\n\
           <parameter name=\"LinearBackground:A1\" type=\"fitting\">\n\
               <formula eq=\"2\" result-unit=\"%2%\"/>\n\
               <fixed />\n\
           </parameter>\n\
           </component-link>\n\
        </parameter-file>\n") % instrumentName % result_unit);

    // Create a temporary Instrument Parameter file.
    ScopedFile file(parameterFileContents, instrumentName + "_Parameters.xml");

    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

  /**
 Helper function, applies fit functions from a fabricated, fake instrument
 parameter file ontop of an existing instrument definition.
 The fit function is set at the instrument level. HOWEVER, this instrument
 definition is corrupted in that at the instrument level, ONLY ONE OF THE TWO
 PARAMETERS FOR LINEARFIT (A0) IS PROVIDED.
*/
  MatrixWorkspace_sptr create_workspace_with_incomplete_instrument_level_fitting_functions() {
    // Create a default workspace with no-fitting functions.
    MatrixWorkspace_sptr ws = create_workspace_with_no_fitting_functions();
    const std::string instrumentName = ws->getInstrument()->getName();

    // Create a parameter file, with a root equation that will apply to all
    // detectors. NOTE THAT A0 IS SPECIFIED, but A1 IS NOT.
    const std::string parameterFileContents = boost::str(boost::format("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
       <parameter-file instrument = \"%1%\" date = \"2012-01-31T00:00:00\">\n\
          <component-link name=\"%1%\">\n\
           <parameter name=\"LinearBackground:A0\" type=\"fitting\">\n\
               <formula eq=\"3\" result-unit=\"Wavelength\"/>\n\
               <fixed />\n\
           </parameter>\n\
           </component-link>\n\
        </parameter-file>\n") % instrumentName);

    // Create a temporary Instrument Parameter file.
    ScopedFile file(parameterFileContents, instrumentName + "_Parameters.xml");

    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

  /**
  Helper function, applies fit functions from a fabricated, fake instrument
  parameter file ontop of an existing instrument definition.
  The fit function is different for every detector.
 */
  MatrixWorkspace_sptr create_workspace_with_detector_level_only_fit_functions() {
    // Create a default workspace with no-fitting functions.
    MatrixWorkspace_sptr ws = create_workspace_with_no_fitting_functions();
    const std::string instrumentName = ws->getInstrument()->getName();

    const double A1 = 1;
    std::string componentLinks = "";

    const auto &spectrumInfo = ws->spectrumInfo();
    for (size_t wsIndex = 0; wsIndex < ws->getNumberHistograms(); ++wsIndex) {
      const auto &detector = spectrumInfo.detector(wsIndex);
      // A0, will vary with workspace index, from detector to detector, A1 is
      // constant = 1.
      componentLinks += boost::str(boost::format("<component-link name=\"%1%\">\n\
           <parameter name=\"LinearBackground:A0\" type=\"fitting\">\n\
               <formula eq=\"%2%\" result-unit=\"Wavelength\"/>\n\
               <fixed />\n\
           </parameter>\n\
           <parameter name=\"LinearBackground:A1\" type=\"fitting\">\n\
               <formula eq=\"%3%\" result-unit=\"Wavelength\"/>\n\
               <fixed />\n\
           </parameter>\n\
           </component-link>\n") % detector.getName() %
                                   wsIndex % A1);
    }

    // Create a parameter file, with a root equation that will apply to all
    // detectors.
    const std::string parameterFileContents =
        std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n") + "<parameter-file instrument = \"" +
        instrumentName + "\" date = \"2012-01-31T00:00:00\">\n" + componentLinks + "</parameter-file>\n";

    // Create a temporary Instrument Parameter file.
    ScopedFile file(parameterFileContents, instrumentName + "_Parameters.xml");

    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

  /**
  Helper function, applies fit functions from a fabricated, fake instrument
  parameter file ontop of an existing instrument definition.
  The fit function is different for every detector. HOWEVER, ONLY ONE OF THE TWO
  PARAMETERS FOR LINEARFIT (A1) IS PROVIDED.
 */
  MatrixWorkspace_sptr create_workspace_with_incomplete_detector_level_only_fit_functions(
      MatrixWorkspace_sptr original = std::shared_ptr<MatrixWorkspace>()) {
    MatrixWorkspace_sptr ws = original;
    if (original == nullptr) {
      // Create a default workspace with no-fitting functions.
      ws = create_workspace_with_no_fitting_functions();
    }
    const std::string instrumentName = ws->getInstrument()->getName();

    std::string componentLinks = "";
    const auto &spectrumInfo = ws->spectrumInfo();

    for (size_t wsIndex = 0; wsIndex < ws->getNumberHistograms(); ++wsIndex) {
      const auto &detector = spectrumInfo.detector(wsIndex);

      // A1, will vary with workspace index. NOTE THAT A0 IS MISSING entirely.
      componentLinks += boost::str(boost::format("<component-link name=\"%1%\">\n\
           <parameter name=\"LinearBackground:A1\" type=\"fitting\">\n\
               <formula eq=\"%2%\" result-unit=\"Wavelength\"/>\n\
               <fixed />\n\
           </parameter>\n\
           </component-link>\n") % detector.getName() %
                                   wsIndex);
    }

    // Create a parameter file, with a root equation that will apply to all
    // detectors.
    const std::string parameterFileContents =
        std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n") + "<parameter-file instrument = \"" +
        instrumentName + "\" date = \"2012-01-31T00:00:00\">\n" + componentLinks + "</parameter-file>\n";

    // Create a temporary Instrument Parameter file.
    ScopedFile file(parameterFileContents, instrumentName + "_Parameters.xml");

    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

  /**
  Helper method for running the algorithm and testing for invalid argument on
  execution.
  */
  void do_test_throws_invalid_argument_on_execution(const MatrixWorkspace_sptr &inputWS, bool parallel = false) {
    NormaliseByDetector alg(parallel);
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("InputWorkspace", inputWS);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByDetectorTest *createSuite() { return new NormaliseByDetectorTest(); }
  static void destroySuite(NormaliseByDetectorTest *suite) { delete suite; }

  void test_catagory() {
    NormaliseByDetector alg;
    TS_ASSERT_EQUALS("CorrectionFunctions\\NormalisationCorrections", alg.category());
  }

  void test_name() {
    NormaliseByDetector alg;
    TS_ASSERT_EQUALS("NormaliseByDetector", alg.name());
  }

  void test_Init() {
    NormaliseByDetector alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_throws_when_no_fit_function_on_detector_tree() {
    MatrixWorkspace_sptr inputWS = create_workspace_with_no_fitting_functions();
    do_test_throws_invalid_argument_on_execution(inputWS);
  }

  void test_throws_when_incomplete_fit_function_on_detector_tree() {
    MatrixWorkspace_sptr inputWS = create_workspace_with_incomplete_instrument_level_fitting_functions();
    do_test_throws_invalid_argument_on_execution(inputWS);
  }

  void test_formula_not_in_wavelength_throws() {
    MatrixWorkspace_sptr inputWS1 = create_workspace_with_fitting_functions("TOF");
    do_test_throws_invalid_argument_on_execution(inputWS1);

    // Sanity check by explicitly setting to Wavelength.
    MatrixWorkspace_sptr inputWS2 = create_workspace_with_fitting_functions("Wavelength");
    do_test_doesnt_throw_on_execution(inputWS2);
  }

  void test_formula_not_specified_doesnt_throw() {
    // If we don't specify wavelength, then we assume the forumlas are working
    // in terms of wavelength.
    MatrixWorkspace_sptr inputWS2 = create_workspace_with_fitting_functions("");
    do_test_doesnt_throw_on_execution(inputWS2);
  }

  void test_parallel_application_throws_nothing() {
    // Linear function 2*x + 1 applied to each x-value.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    const bool parallel = true;
    do_test_doesnt_throw_on_execution(inputWS, parallel);
  }

  void test_sequential_application_throws_nothing() {
    // Linear function 2*x + 1 applied to each x-value.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    const bool parallel = false;
    do_test_doesnt_throw_on_execution(inputWS, parallel);
  }

  void test_workspace_with_instrument_only_fitting_functions() {
    // Linear function 2*x + 1 applied to each x-value. INSTRUMENT LEVEL FIT
    // FUNCTION ONLY.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    // Extract the output workspace so that we can verify the normalisation.
    MatrixWorkspace_sptr outWS = do_test_doesnt_throw_on_execution(inputWS); // EXECUTES THE ALG.

    // Output workspace should have 2 histograms.
    TS_ASSERT_EQUALS(2, outWS->getNumberHistograms());
    // Test the application of the linear function
    for (size_t wsIndex = 0; wsIndex < outWS->getNumberHistograms(); ++wsIndex) {
      const auto &yValues = outWS->y(wsIndex);
      const auto &xValues = outWS->x(wsIndex);
      const auto &eValues = outWS->e(wsIndex);

      TS_ASSERT_EQUALS(3, yValues.size());
      TS_ASSERT_EQUALS(3, eValues.size());
      TS_ASSERT_EQUALS(4, xValues.size());

      const auto &yInputValues = inputWS->y(wsIndex);
      const auto &eInputValues = inputWS->e(wsIndex);

      const auto &wavelength = outWS->points(wsIndex);
      for (size_t binIndex = 0; binIndex < (xValues.size() - 1); ++binIndex) {
        const double expectedValue =
            yInputValues[binIndex] / ((2 * wavelength[binIndex]) + 1); // According to the equation written
                                                                       // into the instrument parameter file
                                                                       // for the instrument component link.
        TS_ASSERT_EQUALS(expectedValue, yValues[binIndex]);
        const double expectedError =
            (eInputValues[binIndex] * expectedValue) / yInputValues[binIndex]; // e = Ein/Ydenom so, since Yout =
                                                                               // Yin/Ydenom, e = Ein * Yout / Yin
        TS_ASSERT_EQUALS(expectedError, eValues[binIndex]);
      }
    }
  }

  void test_compare_sequential_and_parallel_results() {
    // Linear function 2*x + 1 applied to each x-value. INSTRUMENT LEVEL FIT
    // FUNCTION ONLY.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    // Extract the output workspace so that we can verify the normalisation.
    const bool parallel = true;
    const bool sequential = false;
    MatrixWorkspace_sptr outWS_parallel =
        do_test_doesnt_throw_on_execution(inputWS, parallel); // EXECUTES THE ALG IN PARALLEL.
    MatrixWorkspace_sptr outWS_sequential =
        do_test_doesnt_throw_on_execution(inputWS, sequential); // EXECUTES THE ALG SEQUENTIALLY.

    // Output workspaces should have same number of histograms.
    TS_ASSERT_EQUALS(2, outWS_parallel->getNumberHistograms());
    TS_ASSERT_EQUALS(outWS_parallel->getNumberHistograms(), outWS_sequential->getNumberHistograms());

    // Test the application of the linear function
    for (size_t wsIndex = 0; wsIndex < inputWS->getNumberHistograms(); ++wsIndex) {
      const auto &yValuesParallel = outWS_parallel->y(wsIndex);
      const auto &xValuesParallel = outWS_parallel->x(wsIndex);
      const auto &eValuesParallel = outWS_parallel->e(wsIndex);

      const auto &yValuesSequential = outWS_sequential->y(wsIndex);
      const auto &xValuesSequential = outWS_sequential->x(wsIndex);
      const auto &eValuesSequential = outWS_sequential->e(wsIndex);

      // Compare against known sizes.
      TS_ASSERT_EQUALS(3, yValuesParallel.size());
      TS_ASSERT_EQUALS(3, eValuesParallel.size());
      TS_ASSERT_EQUALS(4, xValuesParallel.size());
      // Compare results from different execution types.
      TS_ASSERT_EQUALS(yValuesSequential.size(), yValuesParallel.size());
      TS_ASSERT_EQUALS(xValuesSequential.size(), xValuesParallel.size());
      TS_ASSERT_EQUALS(eValuesSequential.size(), eValuesParallel.size());

      const auto &yInputValues = inputWS->y(wsIndex);
      const auto &xInputValues = inputWS->x(wsIndex);
      const auto &eInputValues = inputWS->e(wsIndex);

      const auto &wavelength = inputWS->points(wsIndex);
      for (size_t binIndex = 0; binIndex < (xInputValues.size() - 1); ++binIndex) {
        const double expectedValue =
            yInputValues[binIndex] / ((2 * wavelength[binIndex]) + 1); // According to the equation written
                                                                       // into the instrument parameter file
                                                                       // for the instrument component link.
        // Compare against the known/calculated value.
        TS_ASSERT_EQUALS(expectedValue, yValuesParallel[binIndex]);
        // Compare results from different execution types.
        TS_ASSERT_EQUALS(yValuesSequential[binIndex], yValuesParallel[binIndex]);
        // Compare the errors calculated.
        const double expectedError =
            (eInputValues[binIndex] * expectedValue) / yInputValues[binIndex]; // e = Ein/Ydenom so, since Yout =
                                                                               // Yin/Ydenom, e = Ein * Yout / Yin
        TS_ASSERT_EQUALS(expectedError, eValuesParallel[binIndex]);
        TS_ASSERT_EQUALS(expectedError, eValuesSequential[binIndex]);
      }
    }
  }

  void test_workspace_with_detector_level_only_fit_functions() {
    // Linear function 1*x + N applied to each x-value, where N is the workspace
    // index. DETECTOR LEVEL FIT FUNCTIONS ONLY.
    MatrixWorkspace_sptr inputWS = create_workspace_with_detector_level_only_fit_functions();
    // Extract the output workspace so that we can verify the normalisation.
    MatrixWorkspace_sptr outWS = do_test_doesnt_throw_on_execution(inputWS); // EXECUTES THE ALG.

    // Output workspace should have 2 histograms.
    TS_ASSERT_EQUALS(2, outWS->getNumberHistograms());
    // Test the application of the linear function
    for (size_t wsIndex = 0; wsIndex < outWS->getNumberHistograms(); ++wsIndex) {
      const auto &yValues = outWS->y(wsIndex);
      const auto &xValues = outWS->x(wsIndex);
      const auto &eValues = outWS->e(wsIndex);

      TS_ASSERT_EQUALS(3, yValues.size());
      TS_ASSERT_EQUALS(3, eValues.size());
      TS_ASSERT_EQUALS(4, xValues.size());

      const auto &yInputValues = inputWS->y(wsIndex);
      const auto &eInputValues = inputWS->e(wsIndex);

      const auto &wavelength = outWS->points(wsIndex);
      for (size_t binIndex = 0; binIndex < (xValues.size() - 1); ++binIndex) {
        const double expectedValue =
            yInputValues[binIndex] /
            ((1 * wavelength[binIndex]) + static_cast<double>(wsIndex)); // According to the equation written
                                                                         // into the instrument parameter
                                                                         // file for the detector component
                                                                         // link.
        const double expectedError =
            (eInputValues[binIndex] * expectedValue) / yInputValues[binIndex]; // e = Ein/Ydenom so, since Yout =
                                                                               // Yin/Ydenom, e = Ein * Yout / Yin
        TS_ASSERT_EQUALS(expectedValue, yValues[binIndex]);
        TS_ASSERT_EQUALS(expectedError, eValues[binIndex]);
      }
    }
  }

  void test_construct_complete_function_from_separate_incomplete_parameters() {
    // With one half - incomplete on the instrument level.
    MatrixWorkspace_sptr incompleteInstrumentLevel =
        create_workspace_with_incomplete_instrument_level_fitting_functions();
    do_test_throws_invalid_argument_on_execution(incompleteInstrumentLevel);

    // With The other half - incomplete on the detector level.
    MatrixWorkspace_sptr incompleteDetectorLevel = create_workspace_with_incomplete_detector_level_only_fit_functions();
    do_test_throws_invalid_argument_on_execution(incompleteInstrumentLevel);

    // Now create and use a complete definition MADE FROM THE TWO INCOMPLETE
    // ONES. Should now have A1 on the detector params and A0 on the instrument
    // params.
    MatrixWorkspace_sptr completeWS =
        create_workspace_with_incomplete_detector_level_only_fit_functions(incompleteInstrumentLevel);
    MatrixWorkspace_sptr outWS = do_test_doesnt_throw_on_execution(completeWS);

    // Output workspace should have 2 histograms.
    TS_ASSERT_EQUALS(2, completeWS->getNumberHistograms());

    // Test the application of the linear function
    for (size_t wsIndex = 0; wsIndex < completeWS->getNumberHistograms(); ++wsIndex) {
      const auto &yValues = outWS->y(wsIndex);
      const auto &xValues = outWS->x(wsIndex);
      const auto &eValues = outWS->e(wsIndex);

      TS_ASSERT_EQUALS(3, yValues.size());
      TS_ASSERT_EQUALS(3, eValues.size());
      TS_ASSERT_EQUALS(4, xValues.size());

      const auto &yInputValues = completeWS->y(wsIndex);
      const auto &eInputValues = completeWS->e(wsIndex);

      const auto &wavelength = outWS->points(wsIndex);
      for (size_t binIndex = 0; binIndex < (xValues.size() - 1); ++binIndex) {
        const double expectedValue =
            yInputValues[binIndex] / ((1 * static_cast<double>(wsIndex) * wavelength[binIndex]) +
                                      3.0); // According to the equation written into the instrument
                                            // parameter file for the detector component link.
        const double expectedError =
            (eInputValues[binIndex] * expectedValue) / yInputValues[binIndex]; // e = Ein/Ydenom so, since Yout =
                                                                               // Yin/Ydenom, e = Ein * Yout / Yin
        TS_ASSERT_EQUALS(expectedValue, yValues[binIndex]);
        TS_ASSERT_EQUALS(expectedError, eValues[binIndex]);
      }
    }
  }
};

/**

Performance Tests

*/
class NormaliseByDetectorTestPerformance : public CxxTest::TestSuite {
public:
  static NormaliseByDetectorTestPerformance *createSuite() { return new NormaliseByDetectorTestPerformance(); }
  static void destroySuite(NormaliseByDetectorTestPerformance *suite) { delete suite; }

  void setUp() override {
    if (!ws) {
      // Load some data
      auto loadalg = AlgorithmManager::Instance().create("Load");
      loadalg->setRethrows(true);
      loadalg->initialize();
      loadalg->setPropertyValue("Filename", "POLREF00004699.nxs");
      loadalg->setPropertyValue("OutputWorkspace", "testws");
      loadalg->execute();

      // Convert units to wavelength
      auto unitsalg = AlgorithmManager::Instance().create("ConvertUnits");
      unitsalg->initialize();
      unitsalg->setPropertyValue("InputWorkspace", "testws");
      unitsalg->setPropertyValue("OutputWorkspace", "testws");
      unitsalg->setPropertyValue("Target", "Wavelength");
      unitsalg->execute();

      // Convert the specturm axis ot signed_theta
      auto specaxisalg = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
      specaxisalg->initialize();
      specaxisalg->setPropertyValue("InputWorkspace", "testws");
      specaxisalg->setPropertyValue("OutputWorkspace", "testws");
      specaxisalg->setPropertyValue("Target", "signed_theta");
      specaxisalg->execute();

      WorkspaceGroup_sptr wsGroup = API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("testws");
      ws = std::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(0));

      const std::string instrumentName = ws->getInstrument()->getName();

      // Create a parameter file, with a root equation that will apply to all
      // detectors.
      const std::string parameterFileContents = boost::str(boost::format(
                                                               "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
        <parameter-file instrument = \"%1%\" date = \"2012-01-31T00:00:00\">\n\
        <component-link name=\"%1%\">\n\
        <parameter name=\"LinearBackground:A0\" type=\"fitting\">\n\
        <formula eq=\"1\" result-unit=\"Wavelength\"/>\n\
        <fixed />\n\
        </parameter>\n\
        <parameter name=\"LinearBackground:A1\" type=\"fitting\">\n\
        <formula eq=\"2\" result-unit=\"Wavelength\"/>\n\
        <fixed />\n\
        </parameter>\n\
        </component-link>\n\
        </parameter-file>\n") % instrumentName);

      // Create a temporary Instrument Parameter file.
      ScopedFile file(parameterFileContents, instrumentName + "_Parameters.xml");

      // Apply parameter file to workspace.
      apply_instrument_parameter_file_to_workspace(ws, file);
    }
  }

  void testSequential() {
    bool parallel = false;
    MatrixWorkspace_sptr normalisedWS = do_test_doesnt_throw_on_execution(ws, parallel);
    // Run some basic sanity checks
    do_basic_checks(normalisedWS);
  }

  void testParallel() {
    bool parallel = true;
    MatrixWorkspace_sptr normalisedWS = do_test_doesnt_throw_on_execution(ws, parallel);
    // Run some basic sanity checks
    do_basic_checks(normalisedWS);
  }

private:
  /// Helper method to run common sanity checks.
  void do_basic_checks(const MatrixWorkspace_sptr &normalisedWS) {
    TS_ASSERT(normalisedWS != nullptr);
    TS_ASSERT(ws->getNumberHistograms() == normalisedWS->getNumberHistograms());
    TS_ASSERT(ws->x(0).size() == normalisedWS->x(0).size());
    TS_ASSERT(ws->y(0).size() == normalisedWS->y(0).size());
    TS_ASSERT(ws->e(0).size() == normalisedWS->e(0).size());
  }

  MatrixWorkspace_sptr ws;
};
