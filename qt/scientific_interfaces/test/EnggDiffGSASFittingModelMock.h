#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffGSASFittingModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffGSASFittingModel : public IEnggDiffGSASFittingModel {

public:
  MOCK_METHOD8(doPawleyRefinement,
               Mantid::API::MatrixWorkspace_sptr(
                   const Mantid::API::MatrixWorkspace_sptr inputWS,
                   const RunLabel &runLabel, const std::string &instParamFile,
                   const std::vector<std::string> &phaseFiles,
                   const std::string &pathToGSASII,
                   const std::string &GSASIIProjectFile, const double dMin,
                   const double negativeWeight));

  MOCK_METHOD6(doRietveldRefinement,
               Mantid::API::MatrixWorkspace_sptr(
                   const Mantid::API::MatrixWorkspace_sptr inputWS,
                   const RunLabel &runLabel, const std::string &instParamFile,
                   const std::vector<std::string> &phaseFiles,
                   const std::string &pathToGSASII,
                   const std::string &GSASIIProjectFile));

  MOCK_CONST_METHOD1(getLatticeParams,
                     boost::optional<Mantid::API::ITableWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getRwp, boost::optional<double>(const RunLabel &runLabel));

  MOCK_CONST_METHOD1(loadFocusedRun, Mantid::API::MatrixWorkspace_sptr(
                                         const std::string &filename));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
