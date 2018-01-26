#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetView {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetView() = default;

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

  /**
   Get fitted peaks workspace corresponding to a given run and bank, if a fit
   has been done on that run
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The workspace, or an empty optional if a fit has not been run
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const = 0;

  /// Get the fitted peaks workspace which has been passed to the view to be
  /// loaded into the widget, in order to pass it to the model
  virtual Mantid::API::MatrixWorkspace_sptr
  getFittedPeaksWorkspaceToAdd() const = 0;

  /// Get the bank ID of the fitted peaks workspace which has been passed to the
  /// view
  virtual size_t getFittedPeaksBankIDToAdd() const = 0;

  /// Get the bank ID requested when getFittedPeaks is run
  virtual size_t getFittedPeaksBankIDToReturn() const = 0;

  /// Get the run number requested when getFittedPeaks is run
  virtual int getFittedPeaksRunNumberToReturn() const = 0;

  /// Get the run number of the fitted peaks workspace which has been passed to
  /// the view
  virtual int getFittedPeaksRunNumberToAdd() const = 0;

  /**
   Get focused workspace corresponding to a given run and bank, if that run has
   been loaded into the widget
   @param runNumber The run number of the run
   @param bank The bank ID of the run
   @return The workspace, or empty optional if that run has not been loaded in
  */
  virtual boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const int runNumber, const size_t bank) const = 0;

  /// Get the focused run which has been passed to the view to be loaded into
  /// the widget, in order to pass it to the model
  virtual Mantid::API::MatrixWorkspace_sptr
  getFocusedWorkspaceToAdd() const = 0;

  /// Get the bank ID of the focused run which has been passed to the view
  virtual size_t getFocusedRunBankIDToAdd() const = 0;

  /// Get the run number of the focused run which has been passed to the view
  virtual int getFocusedRunNumberToAdd() const = 0;

  /// Set the the fitted peaks workspace to be returned when getFittedPeaks is
  /// run on the widget
  virtual void setFittedPeaksWorkspaceToReturn(
      const Mantid::API::MatrixWorkspace_sptr ws) = 0;

  /**
   Display an error message to the user
   @param errorTitle Title of the error
   @param errorDescription Longer description of the error
  */
  virtual void userError(const std::string &errorTitle,
                         const std::string &errorDescription) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
