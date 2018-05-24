#ifndef MARKDEADDETECTORSTEST_H_
#define MARKDEADDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::MantidVecPtr;
using Mantid::detid_t;
using Mantid::specnum_t;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::LinearGenerator;
using Mantid::Types::Event::TofEvent;

class MaskDetectorsTest : public CxxTest::TestSuite {
public:
  static MaskDetectorsTest *createSuite() { return new MaskDetectorsTest(); }
  static void destroySuite(MaskDetectorsTest *suite) { delete suite; }

  MaskDetectorsTest() = default;

  void testName() { TS_ASSERT_EQUALS(marker.name(), "MaskDetectors"); }

  void testVersion() { TS_ASSERT_EQUALS(marker.version(), 1); }

  /*
   * Generate a Workspace which can be (1) EventWorkspace, (2) Workspace2D, and
   * (3) SpecialWorkspace2D
   */
  void setUpWS(bool event, const std::string &name = "testSpace",
               bool asMaskWorkspace = false, int numspec = 9) {
    // 1. Instrument
    int num_banks = numspec / 9;
    if (num_banks < 1)
      num_banks = 1;
    Instrument_sptr instr = boost::dynamic_pointer_cast<Instrument>(
        ComponentCreationHelper::createTestInstrumentCylindrical(num_banks));

    // 2. Workspace
    MatrixWorkspace_sptr space;
    // Set up a small workspace for testing
    if (event) {
      space =
          WorkspaceFactory::Instance().create("EventWorkspace", numspec, 6, 5);
      EventWorkspace_sptr spaceEvent =
          boost::dynamic_pointer_cast<EventWorkspace>(space);
      space->setInstrument(instr);

      MantidVecPtr vec;
      vec.access().resize(5, 1.0);
      std::vector<detid_t> det_ids = instr->getDetectorIDs();
      for (int j = 0; j < numspec; ++j) {
        // Just one event per pixel
        TofEvent event(1.23, int64_t(4.56));
        spaceEvent->getSpectrum(j).addEventQuickly(event);
        spaceEvent->getSpectrum(j).setSpectrumNo(j + 1);
        spaceEvent->getSpectrum(j).setDetectorID(det_ids[j]);
      }
      spaceEvent->setAllX(BinEdges{0.0, 10.0});

    } else if (!asMaskWorkspace) {
      space = createWorkspace<Workspace2D>(numspec, 6, 5);
      space->setInstrument(instr);

      BinEdges x(6, LinearGenerator(10.0, 1.0));
      Counts y(5, 1.0);
      for (size_t j = 0; j < space->getNumberHistograms(); ++j) {
        space->setHistogram(j, x, y);
        space->getSpectrum(j).setSpectrumNo(static_cast<int>(j + 1));
        space->getSpectrum(j).setDetectorID(static_cast<detid_t>(j + 1));
      }
    } else {
      // In case of MaskWorkspace
      MaskWorkspace_sptr specspace(new MaskWorkspace());
      specspace->initialize(numspec, 1, 1);
      for (size_t i = 0; i < specspace->getNumberHistograms(); i++) {
        // default to use all the detectors
        specspace->mutableY(i)[0] = 0.0;
        specspace->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
      }
      space = boost::dynamic_pointer_cast<MatrixWorkspace>(specspace);
      // Does not have connection between instrument and spectra though has to
      // have instrument
      space->setInstrument(instr);
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().addOrReplace(name, space);
  }

  //---------------------------------------------------------------------------------------------
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(marker.initialize());
    TS_ASSERT(marker.isInitialized());

    MaskDetectors mdd;
    TS_ASSERT_THROWS_NOTHING(mdd.initialize());
    TS_ASSERT(mdd.isInitialized());

    std::vector<Property *> props = mdd.getProperties();
    TS_ASSERT_EQUALS(static_cast<int>(props.size()), 9);

    TS_ASSERT_EQUALS(props[0]->name(), "Workspace");
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<Workspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "SpectraList");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<ArrayProperty<specnum_t> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "DetectorList");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<ArrayProperty<detid_t> *>(props[2]));

    TS_ASSERT_EQUALS(props[3]->name(), "WorkspaceIndexList");
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT(dynamic_cast<ArrayProperty<size_t> *>(props[3]));

    TS_ASSERT_EQUALS(props[4]->name(), "MaskedWorkspace");
    TS_ASSERT(props[4]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<> *>(props[4]));

    TS_ASSERT_EQUALS(props[5]->name(), "ForceInstrumentMasking");
    TS_ASSERT(props[5]->isDefault());

    TS_ASSERT_EQUALS(props[6]->name(), "StartWorkspaceIndex");
    TS_ASSERT(props[6]->isDefault());

    TS_ASSERT_EQUALS(props[7]->name(), "EndWorkspaceIndex");
    TS_ASSERT(props[7]->isDefault());

    TS_ASSERT_EQUALS(props[8]->name(), "ComponentList");
    TS_ASSERT(props[8]->isDefault());
    TS_ASSERT(dynamic_cast<ArrayProperty<std::string> *>(props[8]));
  }

  //---------------------------------------------------------------------------------------------
  void testExecWithNoInput() {
    setUpWS(false);

    MaskDetectors masker;

    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("Workspace", "testSpace"));

    TS_ASSERT_THROWS_NOTHING(masker.execute());

    AnalysisDataService::Instance().remove("testSpace");
  }

  void check_outputWS(MatrixWorkspace_const_sptr &outputWS) {
    double ones = 1.0;
    double zeroes = 0.0;
    TS_ASSERT_EQUALS(outputWS->y(0)[0], zeroes);
    TS_ASSERT_EQUALS(outputWS->e(0)[0], zeroes);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], ones);
    TS_ASSERT_EQUALS(outputWS->e(1)[0], ones);
    TS_ASSERT_EQUALS(outputWS->y(2)[0], zeroes);
    TS_ASSERT_EQUALS(outputWS->e(2)[0], zeroes);
    TS_ASSERT_EQUALS(outputWS->y(3)[0], zeroes);
    TS_ASSERT_EQUALS(outputWS->e(3)[0], zeroes);
    TS_ASSERT_EQUALS(outputWS->y(4)[0], ones);
    TS_ASSERT_EQUALS(outputWS->e(4)[0], ones);
    const auto &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT(spectrumInfo.isMasked(0));
    TS_ASSERT(!spectrumInfo.isMasked(1));
    TS_ASSERT(spectrumInfo.isMasked(2));
    TS_ASSERT(spectrumInfo.isMasked(3));
    TS_ASSERT(!spectrumInfo.isMasked(4));
  }

  //---------------------------------------------------------------------------------------------
  void testExec() {
    setUpWS(false);

    if (!marker.isInitialized())
      marker.initialize();

    marker.setPropertyValue("Workspace", "testSpace");

    marker.setPropertyValue("WorkspaceIndexList", "0,3");
    marker.setPropertyValue("DetectorList", "");
    TS_ASSERT_THROWS_NOTHING(marker.execute());

    MaskDetectors marker2;
    marker2.initialize();
    marker2.setPropertyValue("Workspace", "testSpace");
    marker2.setPropertyValue("DetectorList", "");
    marker2.setPropertyValue("SpectraList", "3");
    TS_ASSERT_THROWS_NOTHING(marker2.execute());
    TS_ASSERT(marker2.isExecuted());

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
            "testSpace");
    check_outputWS(outputWS);

    AnalysisDataService::Instance().remove("testSpace");
  }

  //---------------------------------------------------------------------------------------------
  void testExecEventWorkspace() {
    setUpWS(true);

    if (!marker.isInitialized())
      marker.initialize();

    marker.setPropertyValue("Workspace", "testSpace");

    marker.setPropertyValue("WorkspaceIndexList", "0,3");
    marker.setPropertyValue("DetectorList", "");
    TS_ASSERT_THROWS_NOTHING(marker.execute());

    MaskDetectors marker2;
    marker2.initialize();
    marker2.setPropertyValue("Workspace", "testSpace");
    marker2.setPropertyValue("DetectorList", "");
    marker2.setPropertyValue("SpectraList", "3");
    TS_ASSERT_THROWS_NOTHING(marker2.execute());
    TS_ASSERT(marker2.isExecuted());

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
            "testSpace");
    check_outputWS(outputWS);

    AnalysisDataService::Instance().remove("testSpace");
  }

  void test_Mask_Specific_Components() {
    const int numBanks = 3;
    const int numSpec = 9 * numBanks;
    setUpWS(false, "testSpace", false, numSpec);
    MaskDetectors masker;
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.initialize())
    TS_ASSERT(masker.isInitialized())
    TS_ASSERT_THROWS_NOTHING(masker.setProperty("Workspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(masker.setProperty(
        "ComponentList", "bank1/pixel-(0;1), bank3/pixel-(1;1)"))
    const detid_t maskedPixel1 = 7;
    const detid_t maskedPixel2 = 26;
    TS_ASSERT_THROWS_NOTHING(masker.execute())
    TS_ASSERT(masker.isExecuted())
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
            "testSpace");
    const auto &spectrumInfo = outputWS->spectrumInfo();
    for (size_t i = 0; i < numSpec; ++i) {
      if (i == maskedPixel1 || i == maskedPixel2) {
        TS_ASSERT(spectrumInfo.isMasked(i))
      } else {
        TS_ASSERT(!spectrumInfo.isMasked(i))
      }
    }
    AnalysisDataService::Instance().remove("testSpace");
  }

  void test_Mask_Components_Recursively() {
    const int numBanks = 3;
    const int numSpec = 9 * numBanks;
    setUpWS(false, "testSpace", false, numSpec);
    MaskDetectors masker;
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.initialize())
    TS_ASSERT(masker.isInitialized())
    TS_ASSERT_THROWS_NOTHING(masker.setProperty("Workspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(masker.setProperty("ComponentList", "bank2"))
    TS_ASSERT_THROWS_NOTHING(masker.execute())
    TS_ASSERT(masker.isExecuted())
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
            "testSpace");
    const auto &spectrumInfo = outputWS->spectrumInfo();
    for (size_t i = 0; i < numSpec; ++i) {
      if (i >= 9 && i < 18) {
        TS_ASSERT(spectrumInfo.isMasked(i))
      } else {
        TS_ASSERT(!spectrumInfo.isMasked(i))
      }
    }
    AnalysisDataService::Instance().remove("testSpace");
  }

  //---------------------------------------------------------------------------------------------
  void test_That_Giving_A_Workspace_Containing_Masks_Copies_These_Masks_Over() {
    // Create 2 workspaces
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    setUpWS(false, inputWSName);
    setUpWS(false, existingMaskName);
    MatrixWorkspace_sptr existingMask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            existingMaskName);

    // Mask some detectors on the existing mask workspace
    std::set<int> masked_indices;
    masked_indices.insert(1);
    masked_indices.insert(3);
    masked_indices.insert(4);

    auto &detInfo = existingMask->mutableDetectorInfo();
    for (int i = 0; i < static_cast<int>(detInfo.size()); ++i)
      if (masked_indices.count(i) == 1)
        detInfo.setMasked(i, true);

    MaskDetectors masker;
    TS_ASSERT_THROWS_NOTHING(masker.initialize());

    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("Workspace", inputWSName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("MaskedWorkspace", existingMaskName));

    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());

    // Test the original has the correct spectra masked
    MatrixWorkspace_sptr originalWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSName);

    TS_ASSERT(originalWS);
    if (!originalWS)
      return;
    const auto &spectrumInfo = originalWS->spectrumInfo();
    for (int i = 0; i < static_cast<int>(originalWS->getNumberHistograms());
         ++i) {
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (masked_indices.count(i) == 1) {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), true);
        TS_ASSERT_EQUALS(originalWS->y(i)[0], 0.0);
      } else {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), false);
        TS_ASSERT_EQUALS(originalWS->y(i)[0], 1.0);
      }
    }

    // Cleanup
    AnalysisDataService::Instance().remove(inputWSName);
    AnalysisDataService::Instance().remove(existingMaskName);
  }

  /*
   * Test for masking detectors by using a MaskWorkspace
   */
  void test_Using_A_MaskWorkspace() {
    // 1. Create 2 workspaces
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    setUpWS(false, inputWSName);
    setUpWS(false, existingMaskName, true);
    MatrixWorkspace_sptr existingMask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            existingMaskName);
    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSName);

    // 2. Mask some detectors: Mask workspace indexes 0, 3, 4
    std::set<int> masked_indices;
    masked_indices.insert(0);
    masked_indices.insert(3);
    masked_indices.insert(4);
    for (int i = 0; i < static_cast<int>(existingMask->getNumberHistograms());
         i++)
      if (masked_indices.count(i) == 1)
        existingMask->mutableY(i)[0] = 1.0;

    // 3. Set properties and execute
    MaskDetectors masker;
    TS_ASSERT_THROWS_NOTHING(masker.initialize());

    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("Workspace", inputWSName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("MaskedWorkspace", existingMaskName));

    masker.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(masker.execute());

    // 4. Check result by testing the original has the correct spectra masked
    MatrixWorkspace_sptr originalWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSName);

    TS_ASSERT(originalWS);
    if (!originalWS)
      return;

    const auto &spectrumInfo = originalWS->spectrumInfo();
    for (int i = 0; i < static_cast<int>(originalWS->getNumberHistograms() - 1);
         ++i) {
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (masked_indices.count(i) == 1) {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), true);
        TS_ASSERT_EQUALS(originalWS->y(i)[0], 0.0);
      } else {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), false);
        TS_ASSERT_EQUALS(originalWS->y(i)[0], 1.0);
      }
    }
    AnalysisDataService::Instance().remove(inputWSName);
    AnalysisDataService::Instance().remove(existingMaskName);
  }

  void
  test_InputWorkspace_Larger_Than_MaskedWorkspace_Masks_Section_Specified_By_Start_EndWorkspaceIndex() {
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    const int numInputSpec(9);
    setUpWS(false, inputWSName, false, numInputSpec);
    const int numMaskWSSpec(5);
    setUpWS(false, existingMaskName, true, numMaskWSSpec);
    MatrixWorkspace_sptr existingMask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            existingMaskName);
    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSName);
    TS_ASSERT(existingMask);
    TS_ASSERT(inputWS);

    // Mask workspace index 0,3,4 in MaskWS. These will be maped to index 3,5 in
    // the test input
    existingMask->mutableY(0)[0] = 1.0;
    existingMask->mutableY(3)[0] = 1.0;
    existingMask->mutableY(4)[0] = 1.0;

    // Apply
    MaskDetectors masker;
    masker.initialize();
    masker.setPropertyValue("Workspace", inputWSName);
    masker.setPropertyValue("MaskedWorkspace", existingMaskName);
    masker.setPropertyValue("StartWorkspaceIndex", "3");
    masker.setPropertyValue("EndWorkspaceIndex", "5");
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());
    inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        inputWSName);

    // Check masking
    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (int i = 0; i < numInputSpec; ++i) {
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (i == 3 || i == 4) {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), true);
      } else {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), false);
      }
    }

    AnalysisDataService::Instance().remove(inputWSName);
    AnalysisDataService::Instance().remove(existingMaskName);
  }
  //
  void test_rangeMasking() {
    const std::string inputWSName("inputWS");
    int numInputSpec(18);
    setUpWS(false, inputWSName, false, numInputSpec);

    MaskDetectors masker;
    masker.initialize();
    masker.setPropertyValue("Workspace", inputWSName);
    masker.setPropertyValue("StartWorkspaceIndex", "3");
    masker.setPropertyValue("EndWorkspaceIndex", "5");
    masker.execute();

    TS_ASSERT(masker.isExecuted());
    auto inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        inputWSName);

    // Check masking
    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (int i = 0; i < numInputSpec; ++i) {
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (i == 3 || i == 4 || i == 5) {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), true);
      } else {
        TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), false);
      }
    }

    AnalysisDataService::Instance().remove(inputWSName);
  }

  // -- helper function for the next procedure
  void mask_block(MatrixWorkspace_sptr &existingMask, size_t n_first_index,
                  size_t n_dets) {

    for (size_t i = n_first_index; i < n_first_index + n_dets; i++) {
      existingMask->mutableY(i)[0] = 1.0;
    }
  }
  void test_MaskWorksForGroupedWSAllDet() {
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    const int numInputSpec(90);
    setUpWS(false, inputWSName, false, numInputSpec);
    // group spectra into 10
    auto grouper = AlgorithmManager::Instance().create("GroupDetectors");
    grouper->initialize();
    grouper->setProperty("InputWorkspace", inputWSName);
    grouper->setPropertyValue("OutputWorkspace", inputWSName);
    grouper->setPropertyValue(
        "GroupingPattern",
        "0-9,10-19,20-29,30-39,40-49,50-59,60-69,70-79,80-89");
    grouper->execute();
    TS_ASSERT(grouper->isExecuted());

    const int numMaskWSSpec(90);
    setUpWS(false, existingMaskName, true, numMaskWSSpec);
    MatrixWorkspace_sptr existingMask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            existingMaskName);
    // Mask workspace index 0,20,55 in MaskWS. These will converted maped to
    //  indexes 1,2,5 in the target workspace.
    mask_block(existingMask, 0, 10);
    mask_block(existingMask, 20, 10);
    mask_block(existingMask, 50, 10);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSName);
    TS_ASSERT(existingMask);
    TS_ASSERT(inputWS);

    // Apply
    MaskDetectors masker;
    masker.initialize();
    masker.setPropertyValue("Workspace", inputWSName);
    masker.setPropertyValue("MaskedWorkspace", existingMaskName);
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());
    inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        inputWSName);

    // Check masking
    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
      const auto &det = spectrumInfo.detector(i);
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (i == 0 || i == 2 || i == 5) {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), true);
      } else {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), false);
      }
    }

    AnalysisDataService::Instance().remove(inputWSName);
    AnalysisDataService::Instance().remove(existingMaskName);
  }

  void test_MaskWorksForGroupedWSSingleDet() {
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    const int numInputSpec(90);
    setUpWS(false, inputWSName, false, numInputSpec);
    // group spectra into 10
    auto grouper = AlgorithmManager::Instance().create("GroupDetectors");
    grouper->initialize();
    grouper->setProperty("InputWorkspace", inputWSName);
    grouper->setPropertyValue("OutputWorkspace", inputWSName);
    grouper->setPropertyValue(
        "GroupingPattern",
        "0-9,10-19,20-29,30-39,40-49,50-59,60-69,70-79,80-89");
    grouper->execute();
    TS_ASSERT(grouper->isExecuted());

    const int numMaskWSSpec(90);
    setUpWS(false, existingMaskName, true, numMaskWSSpec);
    MatrixWorkspace_sptr existingMask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            existingMaskName);
    // Mask workspace index 1,20,55 in MaskWS. These will converted maped to
    //  indexes 1,2,5 in the target workspace.
    existingMask->mutableY(10)[0] = 1.0;
    existingMask->mutableY(20)[0] = 1.0;
    existingMask->mutableY(55)[0] = 1.0;

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSName);
    TS_ASSERT(existingMask);
    TS_ASSERT(inputWS);

    // Apply
    MaskDetectors masker;
    masker.initialize();
    masker.setPropertyValue("Workspace", inputWSName);
    masker.setPropertyValue("MaskedWorkspace", existingMaskName);
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());
    inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        inputWSName);

    // Check masking
    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
      const auto &det = spectrumInfo.detector(i);
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (i == 1 || i == 2 || i == 5) {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), true);
      } else {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), false);
      }
    }

    AnalysisDataService::Instance().remove(inputWSName);
    AnalysisDataService::Instance().remove(existingMaskName);
  }

  void test_MaskWithWorkspaceWithDetectorIDs() {
    auto &ads = AnalysisDataService::Instance();
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    const int numInputSpec(90);

    setUpWS(false, inputWSName, false, numInputSpec);

    auto inputWS = ads.retrieveWS<MatrixWorkspace>(inputWSName);

    // group spectra into 10
    auto grouper = AlgorithmManager::Instance().create("GroupDetectors");
    grouper->initialize();
    grouper->setProperty("InputWorkspace", inputWSName);
    grouper->setPropertyValue("OutputWorkspace", inputWSName);
    grouper->setPropertyValue(
        "GroupingPattern",
        "0-9,10-19,20-29,30-39,40-49,50-59,60-69,70-79,80-89");
    grouper->execute();

    TS_ASSERT(grouper->isExecuted());

    inputWS = ads.retrieveWS<MatrixWorkspace>(inputWSName);
    TS_ASSERT(inputWS);

    // Make workspace to act as mask
    const auto numMaskWSSpec = inputWS->getInstrument()->getNumberDetectors();
    auto maskWs = WorkspaceCreationHelper::create2DWorkspaceBinned(
        static_cast<int>(numMaskWSSpec), 1, 0., 0.);
    maskWs->setInstrument(inputWS->getInstrument());
    for (size_t i = 0; i < maskWs->getNumberHistograms(); ++i) {
      maskWs->mutableY(i)[0] = 1.0;
      maskWs->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
    }

    maskWs->mutableY(10)[0] = 0;
    maskWs->mutableY(20)[0] = 0;
    maskWs->mutableY(55)[0] = 0;

    // Apply
    MaskDetectors masker;
    masker.initialize();
    masker.setPropertyValue("Workspace", inputWSName);
    masker.setProperty("MaskedWorkspace", maskWs);
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());
    inputWS = ads.retrieveWS<MatrixWorkspace>(inputWSName);

    // Check masking
    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
      const auto &det = spectrumInfo.detector(i);
      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (i == 1 || i == 2 || i == 5) {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), true);
      } else {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), false);
      }
    }
  }

  void test_MaskWithWorkspaceWithDetectorIDsAndWsIndexRange() {
    auto &ads = AnalysisDataService::Instance();
    const std::string inputWSName("inputWS"), existingMaskName("existingMask");
    const int numInputSpec(90);

    setUpWS(false, inputWSName, false, numInputSpec);

    auto inputWS = ads.retrieveWS<MatrixWorkspace>(inputWSName);

    // group spectra into 10
    auto grouper = AlgorithmManager::Instance().create("GroupDetectors");
    grouper->initialize();
    grouper->setProperty("InputWorkspace", inputWSName);
    grouper->setPropertyValue("OutputWorkspace", inputWSName);
    grouper->setPropertyValue(
        "GroupingPattern",
        "0-9,10-19,20-29,30-39,40-49,50-59,60-69,70-79,80-89");
    grouper->execute();

    TS_ASSERT(grouper->isExecuted());

    inputWS = ads.retrieveWS<MatrixWorkspace>(inputWSName);
    TS_ASSERT(inputWS);

    // Make workspace to act as mask
    const auto numMaskWSSpec = inputWS->getInstrument()->getNumberDetectors();
    auto maskWs = WorkspaceCreationHelper::create2DWorkspaceBinned(
        static_cast<int>(numMaskWSSpec), 1, 0., 0.);
    maskWs->setInstrument(inputWS->getInstrument());
    for (size_t i = 0; i < maskWs->getNumberHistograms(); ++i) {
      maskWs->mutableY(i)[0] = 1.0;
      maskWs->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
    }

    maskWs->mutableY(10)[0] = 0;
    maskWs->mutableY(20)[0] = 0;
    maskWs->mutableY(55)[0] = 0;

    // Apply
    MaskDetectors masker;
    masker.initialize();
    masker.setPropertyValue("Workspace", inputWSName);
    masker.setProperty("MaskedWorkspace", maskWs);
    masker.setProperty("StartWorkspaceIndex", 2);
    masker.setProperty("EndWorkspaceIndex", 4);
    masker.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(masker.execute());
    inputWS = ads.retrieveWS<MatrixWorkspace>(inputWSName);

    // Check masking
    const auto &spectrumInfo = inputWS->spectrumInfo();
    for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
      const auto &det = spectrumInfo.detector(i);

      TS_ASSERT(spectrumInfo.hasDetectors(i));
      if (i == 2) {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), true);
      } else {
        TSM_ASSERT_EQUALS("Detector with id: " +
                              boost::lexical_cast<std::string>(det.getID()) +
                              "; Spectra N: " +
                              boost::lexical_cast<std::string>(i),
                          spectrumInfo.isMasked(i), false);
      }
    }
  }

private:
  MaskDetectors marker;
};

#endif /*MARKDEADDETECTORSTEST_H_*/
