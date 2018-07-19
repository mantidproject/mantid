#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_
#define MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_

#include "../EnggDiffraction/IEnggDiffFittingView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffGSASFittingView : public IEnggDiffGSASFittingView {

public:
  MOCK_METHOD1(addWidget, void(IEnggDiffMultiRunFittingWidgetView *widget));

  MOCK_CONST_METHOD1(displayGamma, void(const double gamma));

  MOCK_CONST_METHOD1(displayLatticeParams,
                     void(const Mantid::API::ITableWorkspace_sptr));

  MOCK_CONST_METHOD1(displayRwp, void(const double rwp));

  MOCK_CONST_METHOD1(displaySigma, void(const double sigma));

  MOCK_CONST_METHOD0(getFocusedFileNames, std::vector<std::string>());

  MOCK_CONST_METHOD0(getGSASIIProjectPath, std::string());

  MOCK_CONST_METHOD0(getInstrumentFileName, std::string());

  MOCK_CONST_METHOD0(getPathToGSASII, std::string());

  MOCK_CONST_METHOD0(getPhaseFileNames, std::vector<std::string>());

  MOCK_CONST_METHOD0(getSelectedRunLabel, RunLabel());

  MOCK_CONST_METHOD0(getSelectedRunMethod, GSASRefinementMethod());

  MOCK_CONST_METHOD0(getRefinementMethod, GSASRefinementMethod());

  MOCK_CONST_METHOD0(getRefineGamma, bool());

  MOCK_CONST_METHOD0(getRefineSigma, bool());

  MOCK_CONST_METHOD0(getPawleyDMin, boost::optional<double>());

  MOCK_CONST_METHOD0(getPawleyNegativeWeight, boost::optional<double>());

  MOCK_CONST_METHOD0(getXMax, boost::optional<double>());

  MOCK_CONST_METHOD0(getXMin, boost::optional<double>());

  MOCK_METHOD1(plotCurve,
               void(const std::vector<boost::shared_ptr<QwtData>> &curve));

  MOCK_METHOD0(resetCanvas, void());

  MOCK_METHOD1(setEnabled, void(const bool));

  MOCK_CONST_METHOD0(showRefinementResultsSelected, bool());

  MOCK_CONST_METHOD1(showStatus, void(const std::string &status));

  MOCK_METHOD1(updateRunList, void(const std::vector<RunLabel> &runLabels));

  MOCK_CONST_METHOD2(userError, void(const std::string &errorTitle,
                                     const std::string &errorDescription));

  MOCK_CONST_METHOD2(userWarning, void(const std::string &warningTitle,
                                       const std::string &warningDescription));
};

DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGVIEWMOCK_H_
