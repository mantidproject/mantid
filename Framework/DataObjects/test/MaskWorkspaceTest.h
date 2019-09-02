// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_MASKWORKSPACETEST_H
#define MANTID_DATAOBJECTS_MASKWORKSPACETEST_H

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "PropertyManagerHelper.h"

using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;

class MaskWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() {
    TS_ASSERT_THROWS_NOTHING(Mantid::DataObjects::MaskWorkspace());
  }

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

    Mantid::Geometry::Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
    inst->setName("MaskWorkspaceTest_Instrument");

    Mantid::DataObjects::MaskWorkspace maskWS(inst, false);
    for (int i = 0; i < pixels; i++)
      maskWS.setValue(i, 1); // mask the pixel

    TS_ASSERT_EQUALS(maskWS.getNumberHistograms(), pixels * pixels);
    TS_ASSERT_EQUALS(maskWS.getNumberMasked(), pixels);
    TS_ASSERT(maskWS.isMasked(0));
  }

  void testClone() {
    // As test_mask_accessors(), set on ws, get on clone.
    int pixels = 10;
    int maskpixels = 25;

    Mantid::Geometry::Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
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

    Mantid::Geometry::Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, pixels);
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
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<MaskWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst =
                                 manager.getValue<MaskWorkspace_sptr>(wsName));
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
};

#endif // MANTID_DATAOBJECTS_MASKWORKSPACETEST_H
