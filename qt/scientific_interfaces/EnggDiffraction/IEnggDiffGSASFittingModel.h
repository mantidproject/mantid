#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_

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
   Perform a Pawley refinement on a run
   @param inputWS The workspace to run refinement on
   @param runLabel Run number and bank ID of the workspace to refine
   @param instParamFile The instrument parameter file name (.prm) to use for
   refinement
   @param phaseFiles Vector of file paths to phases to use in refinement
   @param pathToGSASII Location of the directory containing GSASIIscriptable.py
   (and GSAS-II executables)
   @param GSASIIProjectFile Location to save the .gpx project to
   @param dMin The minimum d-spacing to use for refinement
   @param negativeWeight The weight of the penalty function
   @return Fitted peaks workspace resulting from refinement
   @throws If GSASIIRefineFitPeaks throws
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  doPawleyRefinement(const Mantid::API::MatrixWorkspace_sptr inputWS,
                     const RunLabel &runLabel, const std::string &instParamFile,
                     const std::vector<std::string> &phaseFiles,
                     const std::string &pathToGSASII,
                     const std::string &GSASIIProjectFile, const double dMin,
                     const double negativeWeight) = 0;

  /**
   Perform a Rietveld refinement on a run
   @param inputWS The workspace to run refinement on
   @param runLabel Run number and bank ID of the workspace to refine
   @param instParamFile The instrument parameter file name (.prm) to use for
   refinement
   @param phaseFiles Vector of file paths to phases to use in refinement
   @param pathToGSASII Location of the directory containing GSASIIscriptable.py
   (and GSAS-II executables)
   @param GSASIIProjectFile Location to save the .gpx project to
   @return Fitted peaks workspace resulting from refinement
   @throws If GSASIIRefineFitPeaks throws
   */
  virtual Mantid::API::MatrixWorkspace_sptr
  doRietveldRefinement(const Mantid::API::MatrixWorkspace_sptr inputWS,
                       const RunLabel &runLabel,
                       const std::string &instParamFile,
                       const std::vector<std::string> &phaseFiles,
                       const std::string &pathToGSASII,
                       const std::string &GSASIIProjectFile) = 0;

  /**
   Get refined lattice parameters for a run
   @param runLabel Run number and bank ID of the run
   @return TableWorkspace of the corresponding lattice parameters (empty
   optional if the model does not contain fit results for this run)
   */
  virtual boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const RunLabel &runLabel) const = 0;

  /**
   Get the weighted profile R-factor discrepancy index for goodness of fit on a
   run
   @param runLabel Run number and bank ID of the run
   @return The corresponding Rwp value (empty optional if a refinement has not
   been performed on this run)
  */
  virtual boost::optional<double> getRwp(const RunLabel &runLabel) const = 0;

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
