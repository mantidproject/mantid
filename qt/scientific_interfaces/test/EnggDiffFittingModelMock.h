#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H

#include "IEnggDiffractionCalibration.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"

#include "../EnggDiffraction/IEnggDiffFittingModel.h"

#include <gmock/gmock.h>
#include <vector>

using namespace MantidQt::CustomInterfaces;

DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffFittingModel : public IEnggDiffFittingModel {

public:
  MOCK_CONST_METHOD1(getFocusedWorkspace, Mantid::API::MatrixWorkspace_sptr(
                                              const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getAlignedWorkspace, Mantid::API::MatrixWorkspace_sptr(
                                              const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getFittedPeaksWS, Mantid::API::MatrixWorkspace_sptr(
                                           const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getFitResults, Mantid::API::ITableWorkspace_sptr(
                                        const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getWorkspaceFilename,
                     const std::string &(const RunLabel &runLabel));

  MOCK_METHOD1(loadWorkspaces, void(const std::string &filenames));

  MOCK_CONST_METHOD0(getRunLabels, std::vector<RunLabel>());

  MOCK_METHOD2(setDifcTzero,
               void(const RunLabel &runLabel,
                    const std::vector<GSASCalibrationParms> &calibParams));

  MOCK_METHOD2(enggFitPeaks, void(const RunLabel &runLabel,
                                  const std::string &expectedPeaks));

  MOCK_CONST_METHOD2(saveFitResultsToHDF5,
                     void(const std::vector<RunLabel> &runLabels,
                          const std::string &filename));

  MOCK_METHOD1(createFittedPeaksWS, void(const RunLabel &runLabel));

  MOCK_CONST_METHOD0(getNumFocusedWorkspaces, size_t());

  MOCK_CONST_METHOD0(addAllFitResultsToADS, void());

  MOCK_CONST_METHOD0(addAllFittedPeaksToADS, void());

  MOCK_CONST_METHOD1(hasFittedPeaksForRun, bool(const RunLabel &runLabel));

  MOCK_METHOD1(removeRun, void(const RunLabel &runLabel));
};

DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H
