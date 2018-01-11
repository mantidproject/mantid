#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_

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
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @param phaseFiles Vector of file paths to phases to use in refinement
   @param pathToGSASII Location of the directory containing GSASIIscriptable.py
   (and GSAS-II executables)
   @param GSASIIProjectFile Location to save the .gpx project to
   @return Whether the refinement was successful
   */
  virtual bool doPawleyRefinement(const int runNumber, const size_t bank,
                                  const std::string &instParamFile,
                                  const std::vector<std::string> &phaseFiles,
                                  const std::string &pathToGSASII,
                                  const std::string &GSASIIProjectFile,
                                  const double dMin,
                                  const double negativeWeight) = 0;
  /**
   Perform a Rietveld refinement on a run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @param phaseFiles Vector of file paths to phases to use in refinement
   @param pathToGSASII Location of the directory containing GSASIIscriptable.py
   (and GSAS-II executables)
   @param GSASIIProjectFile Location to save the .gpx project to
   @return Whether the refinement was successful
   */
  virtual bool doRietveldRefinement(const int runNumber, const size_t bank,
                                    const std::string &instParamFile,
                                    const std::vector<std::string> &phaseFiles,
                                    const std::string &pathToGSASII,
                                    const std::string &GSASIIProjectFile) = 0;

  /**
   Get the fit results for a given run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return MatrixWorkspace containing the fitted peaks, or empty if the model
   does not contain fit results for this run
   */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  /**
   Get a focused run (as a MatrixWorkspace) by run label
   @param runNumber The runNumber of the run
   @param bank The bank ID of the run
   @return The corresponding workspace (empty if the model does not contain this
   run)
   */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedWorkspace(const int runNumber, const size_t bank) const = 0;

  /**
   Get refined lattice parameters for a run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return TableWorkspace of the corresponding lattice parameters (empty
   optional if the model does not contain fit results for this run)
   */
  virtual boost::optional<Mantid::API::ITableWorkspace_sptr>
  getLatticeParams(const int runNumber, const size_t bank) const = 0;

  /**
   Get run labels (ie run number and bank id) of all runs loaded into model
   @return Vector of all run labels (as pairs)
   */
  virtual std::vector<std::pair<int, size_t>> getRunLabels() const = 0;

  /**
   Get the weighted profile R-factor discrepancy index for goodness of fit on a
   run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The corresponding Rwp value (empty optional if a refinement has not
   been performed on this run)
  */
  virtual boost::optional<double> getRwp(const int runNumber,
                                         const size_t bank) const = 0;

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
