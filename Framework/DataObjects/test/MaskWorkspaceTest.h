// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ExperimentInfo.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "PropertyManagerHelper.h"

using Mantid::API::ExperimentInfo;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoConstIt;

class MaskWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() { TS_ASSERT_THROWS_NOTHING(Mantid::DataObjects::MaskWorkspace()); }

  void test_constructor_using_length() {
    int nDetectors = 10;
    Mantid::DataObjects::MaskWorkspace maskWS(nDetectors);
    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), nDetectors);
    TS_ASSERT_EQUALS(maskWS.blocksize(), 1);
    TS_ASSERT_THROWS(maskWS.getNumberMasked(), const std::runtime_error &);
    TS_ASSERT_THROWS(maskWS.isMasked(0), const std::runtime_error &);
    TS_ASSERT_EQUALS(maskWS.isMaskedIndex(0), false);
    maskWS.setMaskedIndex(0);
    TS_ASSERT_EQUALS(maskWS.isMaskedIndex(0), true);
  }

  void test_constructure_using_instrument() {
    int pixels = 10;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    Mantid::DataObjects::MaskWorkspace maskWS(inst, false);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), 0);
    for (int i = 0; i < pixels; i++)
      maskWS.setMasked(i); // mask the pixel

    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), pixels);
    TS_ASSERT(maskWS.isMasked(0));
  }

  void testClone() {
    // As test_mask_accessors(), set on ws, get on clone.
    int pixels = 10;
    int maskpixels = 25;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    Mantid::DataObjects::MaskWorkspace maskWS(inst, false);
    for (int i = 0; i < maskpixels; i++)
      maskWS.setMasked(i); // mask the pixel

    auto cloned = maskWS.clone();
    TS_ASSERT_EQUALS(cloned->getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(cloned->getNumberMasked(), maskpixels);
    TS_ASSERT(cloned->isMasked(0));
    TS_ASSERT_EQUALS(cloned->isMasked(maskpixels),
                     false); // one past the masked ones

    // unmask a pixel and check it
    maskWS.setMasked(0, false);
    cloned = maskWS.clone();
    TS_ASSERT_EQUALS(cloned->isMasked(0), false);

    // check of a group of pixels
    std::set<Mantid::detid_t> detIds;
    detIds.insert(0); // isn't masked
    TS_ASSERT_EQUALS(cloned->isMasked(detIds), false);
    detIds.insert(1); // is masked
    TS_ASSERT_EQUALS(cloned->isMasked(detIds), false);
    detIds.erase(0);
    detIds.insert(2);
    TS_ASSERT_EQUALS(cloned->isMasked(detIds), true);
  }

  void test_mask_accessors() {
    int pixels = 10;
    int maskpixels = 25;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    Mantid::DataObjects::MaskWorkspace maskWS(inst, false);
    for (int i = 0; i < maskpixels; i++)
      maskWS.setMasked(i); // mask the pixel

    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), maskpixels);
    TS_ASSERT(maskWS.isMasked(0));
    TS_ASSERT_EQUALS(maskWS.isMasked(maskpixels),
                     false); // one past the masked ones

    // unmask a pixel and check it
    maskWS.setMasked(0, false);
    TS_ASSERT_EQUALS(maskWS.isMasked(0), false);

    // check of a group of pixels
    std::set<Mantid::detid_t> detIds;
    detIds.insert(0); // isn't masked
    TS_ASSERT_EQUALS(maskWS.isMasked(detIds), false);
    detIds.insert(1); // is masked
    TS_ASSERT_EQUALS(maskWS.isMasked(detIds), false);
    detIds.erase(0);
    detIds.insert(2);
    TS_ASSERT_EQUALS(maskWS.isMasked(detIds), true);
  }

  /**
   * Test declaring an input MaskWorkspace and retrieving it as const_sptr or
   * sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    MaskWorkspace_sptr wsInput(new Mantid::DataObjects::MaskWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    MaskWorkspace_const_sptr wsConst;
    MaskWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<MaskWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<MaskWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    MaskWorkspace_const_sptr wsCastConst;
    MaskWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (MaskWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (MaskWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  /**
   * Test that workspace creation clears the detector mask flags
   */
  void test_detector_flags_cleared_at_construction() {
    int pixels = 10;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    // Use the ExperimentInfo API to create complete DetectorInfo prior to workspace creation.
    Mantid::API::ExperimentInfo experiment;
    experiment.setInstrument(inst);

    // Set a few of the instrument's detector mask flags
    DetectorInfo &detectors(experiment.mutableDetectorInfo());
    for (int i = 0; i < pixels; i++)
      detectors.setMasked(i, true);

    Mantid::DataObjects::MaskWorkspace maskWS(experiment.getInstrument(), false);
    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), 0);

    const Mantid::Geometry::DetectorInfo &wsDetectors(maskWS.detectorInfo());
    TS_ASSERT(
        std::all_of(wsDetectors.cbegin(), wsDetectors.cend(),
                    [](typename std::iterator_traits<DetectorInfoConstIt>::reference det) { return !det.isMasked(); }));
  }

  /**
   * Test the workspace value to corresponding detector mask consistency check.
   */
  void test_isConsistentWithDetectorMasks() {
    int pixels = 10;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    // Use the ExperimentInfo API to create a complete DetectorInfo prior to workspace creation.
    // With respect to the intended application of these methods, it's important to test using a DetectorInfo
    // object that is distinct from that owned by the MaskWorkspace.
    Mantid::API::ExperimentInfo experiment;
    experiment.setInstrument(inst);
    DetectorInfo &detectors(experiment.mutableDetectorInfo());

    Mantid::DataObjects::MaskWorkspace maskWS(experiment.getInstrument(), false);
    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), 0);

    // mask a few of the pixels
    for (int i = 0; i < pixels; i++)
      TS_ASSERT_THROWS_NOTHING(maskWS.setMaskedIndex(i));
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), pixels);
    TS_ASSERT(!maskWS.isConsistentWithDetectorMasks(detectors));
    // set the corresponding detector mask flags
    for (int i = 0; i < pixels; i++)
      detectors.setMasked(i, true);
    TS_ASSERT(maskWS.isConsistentWithDetectorMasks(detectors));
  }

  /*
   * Test adjustment of workspace values to include detector mask flag values.
   */
  void test_combineFromDetectorMasks() {
    int pixels = 10;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    // Use the ExperimentInfo API to create a complete DetectorInfo prior to workspace creation.
    // With respect to the intended application of these methods, it's important to test using a DetectorInfo
    // object that is distinct from that owned by the MaskWorkspace.
    Mantid::API::ExperimentInfo experiment;
    experiment.setInstrument(inst);
    DetectorInfo &detectors(experiment.mutableDetectorInfo());

    Mantid::DataObjects::MaskWorkspace maskWS(experiment.getInstrument(), false);
    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), 0);

    // mask a few of the detectors
    for (int i = 0; i < pixels; i++)
      detectors.setMasked(i, true);
    TS_ASSERT(!maskWS.isConsistentWithDetectorMasks(detectors));
    TS_ASSERT_THROWS_NOTHING(maskWS.combineFromDetectorMasks(detectors));
    TS_ASSERT(maskWS.isConsistentWithDetectorMasks(detectors));
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), pixels);
  }

  /*
   * Test adjustment of detector mask flags to include workspace values.
   */
  void test_combineToDetectorMasks() {
    int pixels = 10;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    // Use the ExperimentInfo API to create a complete DetectorInfo prior to workspace creation.
    // With respect to the intended application of these methods, it's important to test using a DetectorInfo
    // object that is distinct from that owned by the MaskWorkspace.
    Mantid::API::ExperimentInfo experiment;
    experiment.setInstrument(inst);
    DetectorInfo &detectors(experiment.mutableDetectorInfo());

    Mantid::DataObjects::MaskWorkspace maskWS(experiment.getInstrument(), false);
    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), 0);

    // mask a few of the pixels
    for (int i = 0; i < pixels; i++)
      maskWS.setMasked(i);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), pixels);
    TS_ASSERT(!maskWS.isConsistentWithDetectorMasks(detectors));
    TS_ASSERT_THROWS_NOTHING(maskWS.combineToDetectorMasks(detectors));
    TS_ASSERT(maskWS.isConsistentWithDetectorMasks(detectors));
  }

  /*
   * Test detector-mask flags consistency methods with no instrument.
   */
  void test_maskConsistencyWithNoInstrument() {
    int pixels = 10;

    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    // Use the ExperimentInfo API to create a complete DetectorInfo prior to workspace creation.
    // With respect to the intended application of these methods, it's important to test using a DetectorInfo
    // object that is distinct from that owned by the MaskWorkspace.
    Mantid::API::ExperimentInfo experiment;
    experiment.setInstrument(inst);
    DetectorInfo &detectors(experiment.mutableDetectorInfo());

    Mantid::DataObjects::MaskWorkspace maskWS(pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT(!maskWS.getInstrument() || maskWS.getInstrument()->getNumberDetectors() == 0);

    // All of the consistency methods should throw std::logic_error if called.
    TS_ASSERT_THROWS(maskWS.isConsistentWithDetectorMasks(), const std::logic_error &);

    TS_ASSERT_THROWS(maskWS.combineToDetectorMasks(), const std::logic_error &);

    TS_ASSERT_THROWS(maskWS.combineFromDetectorMasks(), const std::logic_error &);

    TS_ASSERT_THROWS(maskWS.isConsistentWithDetectorMasks(detectors), const std::logic_error &);

    TS_ASSERT_THROWS(maskWS.combineToDetectorMasks(detectors), const std::logic_error &);

    TS_ASSERT_THROWS(maskWS.combineFromDetectorMasks(detectors), const std::logic_error &);
  }
};
