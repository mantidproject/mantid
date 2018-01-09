#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEW_H_

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingView {

public:
  virtual ~IEnggDiffGSASFittingView() = default;

  /**
   Get the name of the focused run file the user has requested to load
   @return Filename to load
   */
  virtual std::string getFocusedFileName() const = 0;

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
