#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_

#include "EnggDiffGSASRefinementMethod.h"
#include "MantidAPI/ITableWorkspace.h"

#include <string>
#include <utility>
#include <vector>

class QwtData;

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingView {

public:
  virtual ~IEnggDiffGSASFittingView() = default;

  /**
   Display lattice parameters to the user
   @param latticeParams TableWorkspace of lattice parameters
  */
  virtual void displayLatticeParams(
      const Mantid::API::ITableWorkspace_sptr latticeParams) const = 0;

  /**
   Display Rwp value to the user
   @param Rwp for the run to display
  */
  virtual void displayRwp(const double rwp) const = 0;

  /**
   Get the name of the focused run file the user has requested to load
   @return Filename to load
   */
  virtual std::string getFocusedFileName() const = 0;

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
   Get the run label (run number and bank id) currently selected in the list
   @return Run label as a pair
   */
  virtual std::pair<int, size_t> getSelectedRunLabel() const = 0;

  /**
   Plot a Qwt curve in the canvas
   @param curve The curve to plot
   */
  virtual void
  plotCurve(const std::vector<boost::shared_ptr<QwtData>> &curve) = 0;

  /**
   Reset canvas to avoid multiple plotting
   */
  virtual void resetCanvas() = 0;

  /**
   @return Whether the user has selected to show the refinement results for
   the current run
  */
  virtual bool showRefinementResultsSelected() = 0;

  /**
   Update the run list with labels of all runs loaded into the model
   @param runLabels Vector of run labels (as pairs)
   */
  virtual void
  updateRunList(const std::vector<std::pair<int, size_t>> &runLabels) = 0;

  /**
   Display a warning to the user
   @param warningDescription The warning to show
   */
  virtual void userWarning(const std::string &warningDescription) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_
