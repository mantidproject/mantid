#ifndef MANTID_DATAOBJECTS_MASKWORKSPACETEST_H
#define MANTID_DATAOBJECTS_MASKWORKSPACETEST_H

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

class MaskWorkspaceTest : public CxxTest::TestSuite
{
public:

    void test_default_constructor()
    {
        TS_ASSERT_THROWS_NOTHING(new Mantid::DataObjects::MaskWorkspace());
    }

    void test_constructor_using_length()
    {
        int nDetectors = 10;
        Mantid::DataObjects::MaskWorkspace* maskWS = new Mantid::DataObjects::MaskWorkspace(nDetectors);
        TS_ASSERT_EQUALS(maskWS->getNumberHistograms(), nDetectors);
        TS_ASSERT_EQUALS(maskWS->blocksize(), 1);
        TS_ASSERT_EQUALS(maskWS->getNumberMasked(), 0);
        TS_ASSERT_THROWS(maskWS->isMasked(0), std::runtime_error);
        TS_ASSERT_EQUALS(maskWS->isMaskedIndex(0), false);
        maskWS->setMaskedIndex(0);
        TS_ASSERT_EQUALS(maskWS->isMaskedIndex(0), true);
    }

    void test_constructure_using_instrument()
    {
        int pixels = 10;

        Mantid::Geometry::Instrument_sptr inst =
                ComponentCreationHelper::createTestInstrumentRectangular2(1,pixels);
        inst->setName("MaskWorkspaceTest_Instrument");

        Mantid::DataObjects::MaskWorkspace* maskWS =
                new Mantid::DataObjects::MaskWorkspace(inst, false);
        for (int i = 0; i < pixels; i++)
          maskWS->setValue(i, 1); // mask the pixel

        TS_ASSERT_EQUALS(maskWS->getNumberHistograms(), pixels*pixels);
        TS_ASSERT_EQUALS(maskWS->getNumberMasked(), pixels);
        TS_ASSERT(maskWS->isMasked(0));
    }

    void test_mask_accessors()
    {
        int pixels = 10;
        int maskpixels = 25;

        Mantid::Geometry::Instrument_sptr inst =
                ComponentCreationHelper::createTestInstrumentRectangular2(1,pixels);
        inst->setName("MaskWorkspaceTest_Instrument");

        Mantid::DataObjects::MaskWorkspace* maskWS =
                new Mantid::DataObjects::MaskWorkspace(inst, false);
        for (int i = 0; i < maskpixels; i++)
          maskWS->setMasked(i); // mask the pixel

        TS_ASSERT_EQUALS(maskWS->getNumberHistograms(), pixels*pixels);
        TS_ASSERT_EQUALS(maskWS->getNumberMasked(), maskpixels);
        TS_ASSERT(maskWS->isMasked(0));
        TS_ASSERT_EQUALS(maskWS->isMasked(maskpixels), false); // one past the masked ones

        // unmask a pixel and check it
        maskWS->setMasked(0, false);
        TS_ASSERT_EQUALS(maskWS->isMasked(0), false);

        // check of a group of pixels
        std::set<Mantid::detid_t> detIds;
        detIds.insert(0); // isn't masked
        TS_ASSERT_EQUALS(maskWS->isMasked(detIds), false);
        detIds.insert(1); // is masked
        TS_ASSERT_EQUALS(maskWS->isMasked(detIds), false);
        detIds.erase(0);
        detIds.insert(2);
        TS_ASSERT_EQUALS(maskWS->isMasked(detIds), true);
    }

};

#endif // MANTID_DATAOBJECTS_MASKWORKSPACETEST_H
