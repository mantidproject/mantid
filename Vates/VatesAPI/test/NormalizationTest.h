#ifndef NORMALIZATIONTEST_H_
#define NORMALIZATIONTEST_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::VATES;

class NormalizationTest : public CxxTest::TestSuite {

public:
  void test_emum_to_enum() {
    // Ensure that enum definitions do not change. They should remain synched.
    TS_ASSERT_EQUALS(static_cast<int>(Mantid::API::NoNormalization),
                     static_cast<int>(Mantid::VATES::NoNormalization));
    TS_ASSERT_EQUALS(static_cast<int>(Mantid::API::VolumeNormalization),
                     static_cast<int>(Mantid::VATES::VolumeNormalization));
    TS_ASSERT_EQUALS(static_cast<int>(Mantid::API::NumEventsNormalization),
                     static_cast<int>(Mantid::VATES::NumEventsNormalization));
  }
};

#endif
