// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/EditInstrumentGeometry.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

using ADS = AnalysisDataService;

class EditInstrumentGeometryTest : public CxxTest::TestSuite {
public:
  /** Test algorithm initialization
   */
  void test_Initialize() {

    EditInstrumentGeometry editdetector;
    TS_ASSERT_THROWS_NOTHING(editdetector.initialize());
    TS_ASSERT(editdetector.isInitialized());
  }

  /** Test `EditInstrumentGeometry` for a workspace containing a single spectrum
   */
  void test_SingleSpectrum() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 100, false);
    ADS::Instance().add("inputWS", inputWS);

    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Workspace", "inputWS"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("SpectrumIDs", "1"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "3.45"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.09"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Azimuthal", "1.84"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ADS::Instance().retrieve("inputWS")));

    const auto &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.hasUniqueDetector(0), true);

    double r, tth, phi;
    spectrumInfo.position(0).getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, 3.45, 0.000001);
    TS_ASSERT_DELTA(tth, 90.09, 0.000001);
    TS_ASSERT_DELTA(phi, 1.84, 0.000001);

    // Clean up
    AnalysisDataService::Instance().remove("inputWS");
  }

  /** Test `EditInstrumentGeometry` using
   *  default values for input spectrum numbers and for
   *  output detector-ids.
   */
  void test_MultipleSpectraDefaultIndices() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);
    ADS::Instance().add("inputWS2", inputWS);

    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Workspace", "inputWS2"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "1.1, 2.2, 3.3, 4.4, 5.5, 6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.1, 90.2, 90.3, 90.4, 90.5, 90.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Azimuthal", "1.0, 2.0, 3.0, 4.0, 5.0, 6.0"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ADS::Instance().retrieve("inputWS2")));

    checkDetectorParameters(outputWS, 0, 1.1, 90.1, 1.0);
    checkDetectorParameters(outputWS, 1, 2.2, 90.2, 2.0);
    checkDetectorParameters(outputWS, 2, 3.3, 90.3, 3.0);
    checkDetectorParameters(outputWS, 3, 4.4, 90.4, 4.0);
    checkDetectorParameters(outputWS, 4, 5.5, 90.5, 5.0);
    checkDetectorParameters(outputWS, 5, 6.6, 90.6, 6.0);

    // For `EditInstrumentGeometry`: the default detector ID values start with 100.
    checkDetectorID(outputWS, 0, 100);
    checkDetectorID(outputWS, 1, 101);
    checkDetectorID(outputWS, 2, 102);
    checkDetectorID(outputWS, 3, 103);
    checkDetectorID(outputWS, 4, 104);
    checkDetectorID(outputWS, 5, 105);

    checkSpectrumNumber(outputWS, 0, 1);
    checkSpectrumNumber(outputWS, 1, 2);
    checkSpectrumNumber(outputWS, 2, 3);
    checkSpectrumNumber(outputWS, 3, 4);
    checkSpectrumNumber(outputWS, 4, 5);
    checkSpectrumNumber(outputWS, 5, 6);

    // Clean up
    AnalysisDataService::Instance().remove("inputWS2");
  }

  //----------------------------------------------------------------------------------------------
  /** Test `EditInstrumentGeometry` using
   *  specified output detector IDs.
   */
  void test_MultipleSpectraNewDetectorIds() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);
    ADS::Instance().add("inputWS3", inputWS);

    // 3. Set Properties
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Workspace", "inputWS3"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "1.1, 2.2, 3.3, 4.4, 5.5, 6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.1, 90.2, 90.3, 90.4, 90.5, 90.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Azimuthal", "1.0, 2.0, 3.0, 4.0, 5.0, 6.0"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("DetectorIDs", "200, 201, 300, 301, 400, 401"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ADS::Instance().retrieve("inputWS3")));

    // Check output detector IDs
    checkDetectorID(outputWS, 0, 200);
    checkDetectorID(outputWS, 1, 201);
    checkDetectorID(outputWS, 2, 300);
    checkDetectorID(outputWS, 3, 301);
    checkDetectorID(outputWS, 4, 400);
    checkDetectorID(outputWS, 5, 401);

    // Clean up
    AnalysisDataService::Instance().remove("inputWS3");
  }

  //----------------------------------------------------------------------------------------------
  /** Test `EditInstrumentGeometry` using
   *  a specified order of the input spectrum numbers.
   */
  void test_MultipleSpectraWithInputSpectrumIds() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);

    // Assign these deliberately out-of-order
    inputWS->getSpectrum(0).setSpectrumNo(specnum_t(93184));
    inputWS->getSpectrum(1).setSpectrumNo(specnum_t(55296));
    inputWS->getSpectrum(2).setSpectrumNo(specnum_t(19456));
    inputWS->getSpectrum(3).setSpectrumNo(specnum_t(74752));
    inputWS->getSpectrum(4).setSpectrumNo(specnum_t(40960));
    inputWS->getSpectrum(5).setSpectrumNo(specnum_t(3072));
    ADS::Instance().add("inputWS4", inputWS);

    // 3. Set Properties
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Workspace", "inputWS4"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("SpectrumIDs", "3072,19456,40960,55296,74752,93184"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "1.1, 2.2, 3.3, 4.4, 5.5, 6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.1, 90.2, 90.3, 90.4, 90.5, 90.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Azimuthal", "1.0, 2.0, 3.0, 4.0, 5.0, 6.0"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ADS::Instance().retrieve("inputWS4")));

    checkDetectorParameters(outputWS, 5, 1.1, 90.1, 1.0);
    checkDetectorParameters(outputWS, 2, 2.2, 90.2, 2.0);
    checkDetectorParameters(outputWS, 4, 3.3, 90.3, 3.0);
    checkDetectorParameters(outputWS, 1, 4.4, 90.4, 4.0);
    checkDetectorParameters(outputWS, 3, 5.5, 90.5, 5.0);
    checkDetectorParameters(outputWS, 0, 6.6, 90.6, 6.0);

    // For `EditInstrumentGeometry`: the default detector ID values start with 100.
    // In this *default* case, regardless of the order of the input spectra,
    //   these are assigned in order of the workspace indices.
    // (TODO: this behavior is almost certainly a _defect_.)
    checkDetectorID(outputWS, 0, 100);
    checkDetectorID(outputWS, 1, 101);
    checkDetectorID(outputWS, 2, 102);
    checkDetectorID(outputWS, 3, 103);
    checkDetectorID(outputWS, 4, 104);
    checkDetectorID(outputWS, 5, 105);

    // Check spectrum numbers:
    //   these should not have changed from the input workspace.
    checkSpectrumNumber(outputWS, 0, specnum_t(93184));
    checkSpectrumNumber(outputWS, 1, specnum_t(55296));
    checkSpectrumNumber(outputWS, 2, specnum_t(19456));
    checkSpectrumNumber(outputWS, 3, specnum_t(74752));
    checkSpectrumNumber(outputWS, 4, specnum_t(40960));
    checkSpectrumNumber(outputWS, 5, specnum_t(3072));

    // Clean up
    AnalysisDataService::Instance().remove("inputWS4");
  }

  //----------------------------------------------------------------------------------------------
  /** Test `EditInstrumentGeometry` using specified output
   *  detector Ids and also a specified order of the input spectrum numbers.
   */
  void test_MultipleSpectraWithInputSpectrumIdsAndNewDetectorIds() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);

    // Assign these deliberately out-of-order
    inputWS->getSpectrum(0).setSpectrumNo(specnum_t(93184));
    inputWS->getSpectrum(1).setSpectrumNo(specnum_t(55296));
    inputWS->getSpectrum(2).setSpectrumNo(specnum_t(19456));
    inputWS->getSpectrum(3).setSpectrumNo(specnum_t(74752));
    inputWS->getSpectrum(4).setSpectrumNo(specnum_t(40960));
    inputWS->getSpectrum(5).setSpectrumNo(specnum_t(3072));
    ADS::Instance().add("inputWS5", inputWS);

    // 3. Set Properties
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Workspace", "inputWS5"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("SpectrumIDs", "3072,19456,40960,55296,74752,93184"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("DetectorIDs", "1001,1002,1003,1004,1005,1006"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "1.1, 2.2, 3.3, 4.4, 5.5, 6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.1, 90.2, 90.3, 90.4, 90.5, 90.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Azimuthal", "1.0, 2.0, 3.0, 4.0, 5.0, 6.0"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ADS::Instance().retrieve("inputWS5")));

    checkDetectorParameters(outputWS, 5, 1.1, 90.1, 1.0);
    checkDetectorParameters(outputWS, 2, 2.2, 90.2, 2.0);
    checkDetectorParameters(outputWS, 4, 3.3, 90.3, 3.0);
    checkDetectorParameters(outputWS, 1, 4.4, 90.4, 4.0);
    checkDetectorParameters(outputWS, 3, 5.5, 90.5, 5.0);
    checkDetectorParameters(outputWS, 0, 6.6, 90.6, 6.0);

    // For `EditInstrumentGeometry`:
    //   the workspace index to spectrum number mapping is not changed.
    // For this non-default case,
    //   each detector ID is assigned to the spectrum at the corresponding
    //   position in the input spectrum-number list.
    checkDetectorID(outputWS, 0, 1006);
    checkDetectorID(outputWS, 1, 1004);
    checkDetectorID(outputWS, 2, 1002);
    checkDetectorID(outputWS, 3, 1005);
    checkDetectorID(outputWS, 4, 1003);
    checkDetectorID(outputWS, 5, 1001);

    // Spectrum numbers should not have changed from those of the input workspace.
    checkSpectrumNumber(outputWS, 0, specnum_t(93184));
    checkSpectrumNumber(outputWS, 1, specnum_t(55296));
    checkSpectrumNumber(outputWS, 2, specnum_t(19456));
    checkSpectrumNumber(outputWS, 3, specnum_t(74752));
    checkSpectrumNumber(outputWS, 4, specnum_t(40960));
    checkSpectrumNumber(outputWS, 5, specnum_t(3072));

    // Clean up
    AnalysisDataService::Instance().remove("inputWS5");
  }

  //----------------------------------------------------------------------------------------------
  /** Test `EditInstrumentGeometry`:
   *    verify the layout of the resulting instrument.
   */
  void test_InstrumentLayout() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100, false);
    ADS::Instance().add("inputWS6", inputWS);

    // 3. Set Properties
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Workspace", "inputWS6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "1.1, 2.2, 3.3, 4.4, 5.5, 6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.1, 90.2, 90.3, 90.4, 90.5, 90.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Azimuthal", "1.0, 2.0, 3.0, 4.0, 5.0, 6.0"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ADS::Instance().retrieve("inputWS6")));

    const auto instrument = outputWS->getInstrument();
    const auto &detectorInfo = outputWS->detectorInfo();
    const auto &componentInfo = outputWS->componentInfo();

    // The instrument includes no monitors
    TS_ASSERT_EQUALS(instrument->getNumberDetectors(true), instrument->getNumberDetectors(false));

    // The instrument has one detector per spectrum
    TS_ASSERT_EQUALS(detectorInfo.size(), outputWS->getNumberHistograms());

    // The instrument includes a source.
    TS_ASSERT(componentInfo.hasSource());

    // The workspace includes a sample.
    TS_ASSERT(componentInfo.hasSample());

    // The component info consists of only these components:
    //   <detectors> + <detector bank> + <source> + <sample> + <root>
    TS_ASSERT_EQUALS(componentInfo.size(), detectorInfo.size() + 4);

    std::optional<size_t> bank = std::nullopt;
    for (size_t index = 0; index < componentInfo.size(); ++index) {
      // Detectors span a contiguous low-index section of the component info.
      if (!componentInfo.isDetector(index))
        break;

      // No detector has the root as its immediate parent.
      // (Changed from previous: this allows `SaveNexusESS` to actually save the instrument.)
      TS_ASSERT(componentInfo.parent(index) != componentInfo.root());

      // There is only one detector bank.
      if (!bank) {
        bank = componentInfo.parent(index);
        // The detector bank's immediate parent is the root.
        TS_ASSERT_EQUALS(componentInfo.parent(*bank), componentInfo.root());
      } else
        // Each detector belongs to the same detector bank.
        TS_ASSERT_EQUALS(componentInfo.parent(index), *bank);
    }

    // Clean up
    AnalysisDataService::Instance().remove("inputWS6");
  }

  //----------------------------------------------------------------------------------------------
  /** Check detector parameters
   */
  void checkDetectorParameters(const API::MatrixWorkspace_sptr &workspace, size_t wsindex, double realr, double realtth,
                               double realphi) {

    const auto &spectrumInfo = workspace->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.hasUniqueDetector(wsindex), true);

    double r, tth, phi;
    spectrumInfo.position(wsindex).getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, realr, 0.000001);
    TS_ASSERT_DELTA(tth, realtth, 0.000001);
    TS_ASSERT_DELTA(phi, realphi, 0.000001);
  }

  //----------------------------------------------------------------------------------------------
  /** Check detector-id
   */
  void checkDetectorID(const API::MatrixWorkspace_sptr &workspace, size_t wsindex, detid_t detid) {

    const auto &spectrumInfo = workspace->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.hasUniqueDetector(wsindex), true);
    TS_ASSERT_EQUALS(spectrumInfo.detector(wsindex).getID(), detid);
  }

  /** Check spectrum number
   */
  void checkSpectrumNumber(const API::MatrixWorkspace_sptr &workspace, size_t wsindex, specnum_t spectrumNumber) {
    TS_ASSERT_EQUALS(workspace->getSpectrum(wsindex).getSpectrumNo(), spectrumNumber);
  }
};
