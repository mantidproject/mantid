#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_

#include "EnggDiffGSASRefinementMethod.h"
#include "IEnggDiffMultiRunFittingWidgetOwner.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"
#include "MantidAPI/ITableWorkspace.h"
#include "RunLabel.h"

#include <string>
#include <utility>
#include <vector>

class QwtData;

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingView : public IEnggDiffMultiRunFittingWidgetOwner {

public:
  virtual ~IEnggDiffGSASFittingView() = default;

  virtual void addWidget(boost::shared_ptr<IEnggDiffMultiRunFittingWidgetView>
                             widget) override = 0;

  /**
   Display lattice parameters to the user
   @param latticeParams TableWorkspace of lattice parameters
  */
  virtual void displayLatticeParams(
      const Mantid::API::ITableWorkspace_sptr latticeParams) const = 0;

  /**
   Display Rwp value to the user
   @param rwp for the run to display
  */
  virtual void displayRwp(const double rwp) const = 0;

  /**
   Get the names of the focused run files the user has requested to load
   @return Vector of filenames to load
   */
  virtual std::vector<std::string> getFocusedFileNames() const = 0;

  /**
   Get the path to export the created GSAS-II project file to
   @return GSAS-II project path
   */
  virtual std::string getGSASIIProjectPath() const = 0;

  /**
   Get path to the instrument parameter file selected by the user for refinement
   @return Instrument file path
   */
  virtual std::string getInstrumentFileName() const = 0;

  /**
   Get path to the directory containing the GSAS-II API
   @return Path to GSAS-II installation directory
   */
  virtual std::string getPathToGSASII() const = 0;

  /**
   @return The minimum d-spacing to use in Pawley refinement
   */
  virtual double getPawleyDMin() const = 0;

  /**
   @return The weight for penalty function applied during Pawley refinement
   */
  virtual double getPawleyNegativeWeight() const = 0;

  /**
   Get paths to all phase files selected for refinement
   @return Phase file paths
   */
  virtual std::vector<std::string> getPhaseFileNames() const = 0;

  /**
   Get the selected refinement method (Pawley or Rietveld)
   @return Refinement method
   */
  virtual GSASRefinementMethod getRefinementMethod() const = 0;

  /**
   Display an error to the user
   @param errorTitle Title of the error
   @param errorDescription Longer description of the error
  */
  virtual void userError(const std::string &errorTitle,
                         const std::string &errorDescription) const = 0;

  /**
   Display a warning to the user
   @param warningTitle Title of the warning
   @param warningDescription Longer description of the warning
   */
  virtual void userWarning(const std::string &warningTitle,
                           const std::string &warningDescription) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_
