#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffGSASFittingModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffGSASFittingModel
    : public MantidQt::CustomInterfaces::IEnggDiffGSASFittingModel {

public:
  MOCK_METHOD8(doPawleyRefinement,
               bool(const int runNumber, const size_t bank,
                    const std::string &instParamFile,
                    const std::vector<std::string> &phaseFiles,
                    const std::string &pathToGSASII,
                    const std::string &GSASIIProjectFile, const double dMin,
                    const double negativeWeight));

  MOCK_METHOD6(doRietveldRefinement,
               bool(const int runNumber, const size_t bank,
                    const std::string &instParamFile,
                    const std::vector<std::string> &phaseFiles,
                    const std::string &pathToGSASII,
                    const std::string &GSASIIProjectFile));

  MOCK_CONST_METHOD2(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD2(getFocusedWorkspace,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD2(getLatticeParams,
                     boost::optional<Mantid::API::ITableWorkspace_sptr>(
                         const int runNumber, const size_t bank));

  MOCK_CONST_METHOD0(getRunLabels, std::vector<std::pair<int, size_t>>());

  MOCK_CONST_METHOD2(getRwp, boost::optional<double>(const int runNumber,
                                                     const size_t bank));

  MOCK_CONST_METHOD2(hasFittedPeaksForRun,
                     bool(const int runNumber, const size_t bank));

  MOCK_METHOD1(loadFocusedRun, std::string(const std::string &filename));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
