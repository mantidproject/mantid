#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_

#include "../EnggDiffraction/EnggDiffMultiRunFittingWidgetModel.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class EnggDiffMultiRunFittingWidgetModelTest : public CxxTest::TestSuite {
public:
  void test_addAndGetFittedPeaks() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);
    TS_ASSERT_THROWS_NOTHING(model.addFittedPeaks(123, 1, ws));

    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(123, 1));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);

    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(456, 2));
    TS_ASSERT(!retrievedWS);
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_
