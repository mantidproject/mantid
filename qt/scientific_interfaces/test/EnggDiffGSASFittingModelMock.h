#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffGSASFittingModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffGSASFittingModel : public IEnggDiffGSASFittingModel {

public:
  MOCK_METHOD7(doPawleyRefinement,
               bool(const RunLabel &runLabel, const std::string &instParamFile,
                    const std::vector<std::string> &phaseFiles,
                    const std::string &pathToGSASII,
                    const std::string &GSASIIProjectFile, const double dMin,
                    const double negativeWeight));

  MOCK_METHOD5(doRietveldRefinement,
               bool(const RunLabel &runLabel, const std::string &instParamFile,
                    const std::vector<std::string> &phaseFiles,
                    const std::string &pathToGSASII,
                    const std::string &GSASIIProjectFile));

  MOCK_CONST_METHOD1(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getFocusedWorkspace,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getLatticeParams,
                     boost::optional<Mantid::API::ITableWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD0(getRunLabels, std::vector<RunLabel>());

  MOCK_CONST_METHOD1(getRwp, boost::optional<double>(const RunLabel &runLabel));

  MOCK_CONST_METHOD1(hasFittedPeaksForRun, bool(const RunLabel &runLabel));

  MOCK_METHOD1(loadFocusedRun, bool(const std::string &filename));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
