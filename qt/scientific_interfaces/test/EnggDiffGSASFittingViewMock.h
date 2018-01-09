#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_
#define MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffFittingView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffGSASFittingView
    : public MantidQt::CustomInterfaces::IEnggDiffGSASFittingView {

public:
  MOCK_CONST_METHOD0(getFocusedFileName, std::string());

  MOCK_METHOD1(updateRunList,
               void(const std::vector<std::pair<int, size_t>> &runLabels));

  MOCK_CONST_METHOD1(userWarning, void(const std::string &warningDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_
