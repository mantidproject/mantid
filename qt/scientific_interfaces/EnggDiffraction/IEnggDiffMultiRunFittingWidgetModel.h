#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetModel {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetModel() = default;

  /**
   Add a fitted peaks workspace to the widget, so it can be overplotted on its
   focused run
   @param runNumber The run number of the workspace to add
   @param bank The bank ID of the workspace to add
   @param ws The workspace to add
  */
  virtual void addFittedPeaks(const int runNumber, const size_t bank,
                              const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /**
   Add a focused run to the widget. The run should be added to the list and
   plotting it should be possible
   @param runNumber The run number of the workspace to add
   @param bank The bank ID of the workspace to add
   @param ws The workspace to add
  */
  virtual void addFocusedRun(const int runNumber, const size_t bank,
                             const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /// Get run numbers and bank IDs of all runs loaded into the model
  virtual std::vector<std::pair<int, size_t>> getAllWorkspaceLabels() const = 0;

  /**
   Get fitted peaks workspace corresponding to a given run and bank, if a fit
   has been done on that run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The workspace, or an empty optional if a fit has not been run
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  /**
   Get focused workspace corresponding to a given run and bank, if that run has
   been loaded into the widget
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The workspace, or empty optional if that run has not been loaded in
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const int runNumber, const size_t bank) const = 0;
};

} // namespace MantidQt
} // namespace CustomInterfaces

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETMODEL_H_
