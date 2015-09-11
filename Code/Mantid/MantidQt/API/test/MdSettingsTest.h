#ifndef MANTID_QT_API_MD_SETTINGS_TEST_H
#define MANTID_QT_API_MD_SETTINGS_TEST_H
#include <cxxtest/TestSuite.h>
#include "MantidQtAPI/MdSettings.h"
#include "MantidQtAPI/MdConstants.h"
#include "MantidAPI/IMDWorkspace.h"

using namespace MantidQt::API;

class MdSettingsTest : public CxxTest::TestSuite {
public:
  void testThatCorrectNormalizationIntegerIsReturned() {
    MdSettings settings;
    MdConstants constants;
    // Ensure that enum definitions do not change. They should remain
    // synched.Ideally they would be
    // sychned against the enum in the VATES API, but this would introduce an
    // extra dependency.
    TS_ASSERT_EQUALS(
        static_cast<int>(Mantid::API::NoNormalization),
        settings.convertNormalizationToInteger(constants.getNoNormalization()));
    TS_ASSERT_EQUALS(static_cast<int>(Mantid::API::VolumeNormalization),
                     settings.convertNormalizationToInteger(
                         constants.getVolumeNormalization()));
    TS_ASSERT_EQUALS(static_cast<int>(Mantid::API::NumEventsNormalization),
                     settings.convertNormalizationToInteger(
                         constants.getNumberEventNormalization()));
    TS_ASSERT_EQUALS(3, settings.convertNormalizationToInteger(
                            constants.getAutoNormalization()));
  }
};

#endif
