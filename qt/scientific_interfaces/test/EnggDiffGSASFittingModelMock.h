#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffGSASFittingModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffGSASFittingModel : public IEnggDiffGSASFittingModel {

public:
  MOCK_METHOD1(doRefinements,
               void(const std::vector<GSASIIRefineFitPeaksParameters> &params));

  MOCK_CONST_METHOD1(getGamma,
                     boost::optional<double>(const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getLatticeParams,
                     boost::optional<Mantid::API::ITableWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getRwp, boost::optional<double>(const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getSigma,
                     boost::optional<double>(const RunLabel &runLabel));

  MOCK_CONST_METHOD1(hasFitResultsForRun, bool(const RunLabel &runLabel));

  MOCK_CONST_METHOD1(loadFocusedRun, Mantid::API::MatrixWorkspace_sptr(
                                         const std::string &filename));

  MOCK_CONST_METHOD3(
      saveRefinementResultsToHDF5,
      void(Mantid::API::IAlgorithm_sptr successfulAlgorithm,
           const std::vector<GSASIIRefineFitPeaksOutputProperties> &
               refinementResultSets,
           const std::string &filename));

  MOCK_METHOD1(setObserver,
               void(boost::shared_ptr<IEnggDiffGSASFittingObserver> observer));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H_
