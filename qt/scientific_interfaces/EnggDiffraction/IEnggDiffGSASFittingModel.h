#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_

#include "GSASIIRefineFitPeaksParameters.h"
#include "RunLabel.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/optional.hpp>

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingModel {

public:
  virtual ~IEnggDiffGSASFittingModel() = default;

  /**
   Perform a refinement on a run
   @param params Parameters to be passed to GSASIIRefineFitPeaks
   @return Fitted peaks workspace resulting from refinement
   @throws If GSASIIRefineFitPeaks throws
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  doRefinement(const GSASIIRefineFitPeaksParameters &params) = 0;

  /**
   Get refined lattice parameters for a run
   @param runLabel Run number and bank ID of the run
   @return TableWorkspace of the corresponding lattice parameters (empty
   optional if the model does not contain fit results for this run)
   */
  virtual boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const RunLabel &runLabel) const = 0;

  /// Get gamma peak broadening term for a given run, if a fit has been done on
  /// that run
  virtual boost::optional<double> getGamma(const RunLabel &runLabel) const = 0;

  /**
   Get the weighted profile R-factor discrepancy index for goodness of fit on a
   run
   @param runLabel Run number and bank ID of the run
   @return The corresponding Rwp value (empty optional if a refinement has not
   been performed on this run)
  */
  virtual boost::optional<double> getRwp(const RunLabel &runLabel) const = 0;

  /// Get sigma peak broadening term for a given run, if a fit has been done on
  /// that run
  virtual boost::optional<double> getSigma(const RunLabel &runLabel) const = 0;

  /// Get whether the model contains fit results for a given run
  virtual bool hasFitResultsForRun(const RunLabel &runLabel) const = 0;

  /**
   Load a focused run from a file
   @param filename The name of the file to load
   @return The loaded workspace
   @throws If Load throws
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  loadFocusedRun(const std::string &filename) const = 0;
};

} // namespace MantidQt
} // namespace CustomInterfaces

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
