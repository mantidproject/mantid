#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffGSASFittingModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffGSASFittingModel
    : public MantidQt::CustomInterfaces::IEnggDiffGSASFittingModel {

public:
  MOCK_CONST_METHOD2(getFocusedWorkspace,
                     Mantid::API::MatrixWorkspace_sptr(const int runNumber,
                                                       const size_t bank));

  MOCK_CONST_METHOD0(getRunLabels, std::vector<std::pair<int, size_t>>());

  MOCK_METHOD1(loadFocusedRun, std::string(const std::string &filename));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
