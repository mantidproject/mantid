#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "IEnggDiffractionCalibration.h"

#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffFittingModel {

public:
  virtual ~IEnggDiffFittingModel() = default;

  virtual Mantid::API::MatrixWorkspace_sptr
  getFocusedWorkspace(const int runNumber, const size_t bank) const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr
  getAlignedWorkspace(const int runNumber, const size_t bank) const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr
  getFittedPeaksWS(const int runNumber, const size_t bank) const = 0;

  virtual Mantid::API::ITableWorkspace_sptr
  getFitResults(const int runNumber, const size_t bank) const = 0;

  virtual const std::string &getWorkspaceFilename(const int runNumber,
                                                  const size_t bank) const = 0;

  virtual void removeRun(const int runNumber, const size_t bank) = 0;

  virtual void loadWorkspaces(const std::string &filenames) = 0;

  virtual std::vector<std::pair<int, size_t>>
  getRunNumbersAndBankIDs() const = 0;

  virtual void
  setDifcTzero(const int runNumber, const size_t bank,
               const std::vector<GSASCalibrationParms> &calibParams) = 0;

  virtual void enggFitPeaks(const int runNumber, const size_t bank,
                            const std::string &expectedPeaks) = 0;

  virtual void saveDiffFittingAscii(const int runNumber, const size_t bank,
                                    const std::string &filename) const = 0;

  virtual void createFittedPeaksWS(const int runNumber, const size_t bank) = 0;

  virtual size_t getNumFocusedWorkspaces() const = 0;

  virtual void addAllFitResultsToADS() const = 0;

  virtual void addAllFittedPeaksToADS() const = 0;

  virtual bool hasFittedPeaksForRun(const int runNumber,
                                    const size_t bank) const = 0;
};

} // namespace MantidQt
} // namespace CustomInterfaces

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGMODEL_H_
