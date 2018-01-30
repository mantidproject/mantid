#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_
#define MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffFittingView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffGSASFittingView
    : public MantidQt::CustomInterfaces::IEnggDiffGSASFittingView {

public:
  MOCK_CONST_METHOD1(displayLatticeParams,
                     void(const Mantid::API::ITableWorkspace_sptr));

  MOCK_CONST_METHOD1(displayRwp, void(const double rwp));

  MOCK_CONST_METHOD0(getFocusedFileName, std::string());

  MOCK_CONST_METHOD0(getGSASIIProjectPath, std::string());

  MOCK_CONST_METHOD0(getInstrumentFileName, std::string());

  MOCK_CONST_METHOD0(getPathToGSASII, std::string());

  MOCK_CONST_METHOD0(getPhaseFileNames, std::vector<std::string>());

  MOCK_CONST_METHOD0(getSelectedRunLabel, std::pair<int, size_t>());

  MOCK_CONST_METHOD0(getSelectedRunMethod,
                     MantidQt::CustomInterfaces::GSASRefinementMethod());

  MOCK_CONST_METHOD0(getRefinementMethod,
                     MantidQt::CustomInterfaces::GSASRefinementMethod());

  MOCK_CONST_METHOD0(getPawleyDMin, double());

  MOCK_CONST_METHOD0(getPawleyNegativeWeight, double());

  MOCK_METHOD1(plotCurve,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD0(resetCanvas, void());

  MOCK_METHOD0(showRefinementResultsSelected, bool());

  MOCK_METHOD1(updateRunList,
               void(const std::vector<std::pair<int, size_t>> &runLabels));

  MOCK_CONST_METHOD1(userWarning, void(const std::string &warningDescription));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_
