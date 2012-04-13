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
    }

    void test_constructure_using_instrument()
    {
        int pixels = 10;

        Mantid::Geometry::Instrument_sptr inst =
                ComponentCreationHelper::createTestInstrumentRectangular2(1,pixels);
        inst->setName("MaskWorkspaceTest_Instrument");

        Mantid::DataObjects::MaskWorkspace* maskWS =
                new Mantid::DataObjects::MaskWorkspace(inst, false);

        TS_ASSERT_EQUALS(maskWS->getNumberHistograms(), pixels*pixels);

    }

};

#endif // MANTID_DATAOBJECTS_MASKWORKSPACETEST_H
