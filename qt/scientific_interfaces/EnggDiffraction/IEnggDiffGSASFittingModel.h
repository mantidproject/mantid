#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingModel {

public:
  virtual ~IEnggDiffGSASFittingModel() = default;

  /**
   Get the fit results for a given run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return MatrixWorkspace containing the fitted peaks
   @throws If the model does not contain fit results for this run
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  /**
   Get a focused run (as a MatrixWorkspace) by run label
   @param runNumber The runNumber of the run
   @param bank The bank ID of the run
   @return The corresponding workspace
   @throws If the run is not found in the model
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  getFocusedWorkspace(const int runNumber, const size_t bank) const = 0;

  /**
   Get run labels (ie run number and bank id) of all runs loaded into model
   @return Vector of all run labels (as pairs)
   */
  virtual std::vector<std::pair<int, size_t>> getRunLabels() const = 0;

  /**
   Has a fit been performed on a given run?
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return Whether the model contains the results of a fit for this run
   */
  virtual bool hasFittedPeaksForRun(const int runNumber,
                                    const size_t bank) const = 0;

  /**
   Load a focused run from a file to the model
   @param filename The name of the file to load
   @return String describing why the load was a failure (empty if success)
   */
  virtual std::string loadFocusedRun(const std::string &filename) = 0;
};

} // namespace MantidQt
} // namespace CustomInterfaces

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
