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

GCC_DIAG_OFF_SUGGEST_OVERRIDE

<<<<<<< d0e8bd66971df0265b39b58a06561a03cb1ebfb0
class MockEnggDiffFittingModel : public IEnggDiffFittingModel {

public:
  MOCK_CONST_METHOD2(getFocusedWorkspace,
                     Mantid::API::MatrixWorkspace_sptr(const int runNumber,
                                                       const size_t bank));

  MOCK_CONST_METHOD2(getAlignedWorkspace,
                     Mantid::API::MatrixWorkspace_sptr(const int runNumber,
                                                       const size_t bank));

  MOCK_CONST_METHOD2(getFittedPeaksWS,
                     Mantid::API::MatrixWorkspace_sptr(const int runNumber,
                                                       const size_t bank));

  MOCK_CONST_METHOD2(getFitResults,
                     Mantid::API::ITableWorkspace_sptr(const int runNumber,
                                                       const size_t bank));

  MOCK_CONST_METHOD2(getWorkspaceFilename,
                     std::string(const int runNumber, const size_t bank));

  MOCK_METHOD1(loadWorkspaces, void(const std::string &filenames));

  MOCK_CONST_METHOD0(getRunNumbersAndBankIDs,
                     std::vector<std::pair<int, size_t>>());

  MOCK_METHOD3(setDifcTzero,
               void(const int runNumber, const size_t bank,
                    const std::vector<GSASCalibrationParms> &calibParams));

  MOCK_METHOD3(enggFitPeaks, void(const int runNumber, const size_t bank,
                                  const std::string &expectedPeaks));

  MOCK_CONST_METHOD3(saveDiffFittingAscii,
                     void(const int runNumber, const size_t bank,
                          const std::string &filename));

  MOCK_METHOD2(createFittedPeaksWS,
               void(const int runNumber, const size_t bank));
=======
class MockEnggDiffFittingModel: public IEnggDiffFittingModel {

public:

	MOCK_CONST_METHOD2(getFocusedWorkspace,
		               Mantid::API::MatrixWorkspace_sptr(const int runNumber,
		   	                                             const size_t bank));

	MOCK_CONST_METHOD2(getAlignedWorkspace,
		               Mantid::API::MatrixWorkspace_sptr(const int runNumber,
			                                             const size_t bank));

	MOCK_CONST_METHOD2(getFittedPeaksWS,
		Mantid::API::MatrixWorkspace_sptr(const int runNumber,
			const size_t bank));

	MOCK_CONST_METHOD2(getFitResults,
		Mantid::API::ITableWorkspace_sptr(const int runNumber,
			const size_t bank));

	MOCK_CONST_METHOD2(getWorkspaceFilename,
		std::string(const int runNumber, const size_t bank));

	MOCK_METHOD1(loadWorkspaces, void(const std::string &filenames));

	MOCK_CONST_METHOD0(getRunNumbersAndBankIDs,
		               std::vector<std::pair<int, size_t>>());

	MOCK_METHOD3(setDifcTzero,
		         void(const int runNumber, const size_t bank,
			          const std::vector<GSASCalibrationParms> &calibParams));

	MOCK_METHOD3(enggFitPeaks,
		         void(const int runNumber, const size_t bank,
			          const std::string &expectedPeaks));

	MOCK_CONST_METHOD3(saveDiffFittingAscii,
		               void(const int runNumber, const size_t bank,
			                const std::string &filename));

	MOCK_METHOD2(createFittedPeaksWS, 
		         void(const int runNumber, const size_t bank));
>>>>>>> Re #21171 Implement mock for enggdifffittingmodel
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFFITTINGMODELMOCK_H