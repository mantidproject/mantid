#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_

#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>
#include <vector>

class QwtData;

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffMultiRunFittingWidgetView {
public:
  virtual ~IEnggDiffMultiRunFittingWidgetView() = default;

  /// Get run number and bank ID of the run currently selected in the list
  virtual std::pair<int, size_t> getSelectedRunLabel() const = 0;

  /// Plot a Qwt curve representing a fitted peaks workspace to the canvas
  virtual void
  plotFittedPeaks(const std::vector<boost::shared_ptr<QwtData>> &curve) = 0;

  /// Plot a Qwt curve representing a focused run to the canvas
  virtual void
  plotFocusedRun(const std::vector<boost::shared_ptr<QwtData>> &curve) = 0;

  /// Clear the plot area to avoid overplotting
  virtual void resetCanvas() = 0;

  /// Get whether the user has selected to overplot fit results
  virtual bool showFitResultsSelected() const = 0;

  /// Update the list of loaded run numbers and bank IDs
  virtual void
  updateRunList(const std::vector<std::pair<int, size_t>> &runLabels) = 0;

  /// Report an error to the user
  virtual void userError(const std::string &errorTitle,
                         const std::string &errorDescription) = 0;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
