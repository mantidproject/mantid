// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_

#include "GSASIIRefineFitPeaksParameters.h"
#include "IEnggDiffGSASFittingObserver.h"
#include "RunLabel.h"

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingModel {

public:
  virtual ~IEnggDiffGSASFittingModel() = default;

  /**
   Perform refinements on a number of runs
   @param params Parameters for each run to be passed to GSASIIRefineFitPeaks
   */
  virtual void
  doRefinements(const std::vector<GSASIIRefineFitPeaksParameters> &params) = 0;

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

  /**
   Save results of refinement (and refinement settings used) to HDF5 file
   @param successfulAlgorithm The completed refinement algorithm (note, even in
   a multi-run fit, input properties like RefineSigma will be the same for all
   runs, so it's fine to just use one algorithm)
   @param refinementResultSets Output properties of all refinements from a
   multi-run fit
   @param filename Name of the HDF5 file to save to
  */
  virtual void saveRefinementResultsToHDF5(
      const Mantid::API::IAlgorithm_sptr successfulAlgorithm,
      const std::vector<GSASIIRefineFitPeaksOutputProperties>
          &refinementResultSets,
      const std::string &filename) const = 0;

  /// set the observer for refinement
  virtual void
  setObserver(boost::shared_ptr<IEnggDiffGSASFittingObserver> observer) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
