#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGMODEL_H_

#include "RunLabel.h"

#include "DllConfig.h"
#include "IEnggDiffractionCalibration.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffFittingModel {

public:
  virtual ~IEnggDiffFittingModel() = default;

  virtual Mantid::API::MatrixWorkspace_sptr
  getFocusedWorkspace(const RunLabel &runLabel) const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr
  getAlignedWorkspace(const RunLabel &runLabel) const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr
  getFittedPeaksWS(const RunLabel &runLabel) const = 0;

  virtual Mantid::API::ITableWorkspace_sptr
  getFitResults(const RunLabel &runLabel) const = 0;

  virtual const std::string &
  getWorkspaceFilename(const RunLabel &runLabel) const = 0;

  virtual void removeRun(const RunLabel &runLabel) = 0;

  virtual void loadWorkspaces(const std::string &filenames) = 0;

  virtual std::vector<RunLabel> getRunLabels() const = 0;

  virtual void
  setDifcTzero(const RunLabel &runLabel,
               const std::vector<GSASCalibrationParms> &calibParams) = 0;

  virtual void enggFitPeaks(const RunLabel &runLabel,
                            const std::string &expectedPeaks) = 0;

  virtual void saveFitResultsToHDF5(const std::vector<RunLabel> &runLabel,
                                    const std::string &filename) const = 0;

  virtual void createFittedPeaksWS(const RunLabel &runLabel) = 0;

  virtual size_t getNumFocusedWorkspaces() const = 0;

  virtual void addAllFitResultsToADS() const = 0;

  virtual void addAllFittedPeaksToADS() const = 0;

  virtual bool hasFittedPeaksForRun(const RunLabel &runLabel) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGMODEL_H_
