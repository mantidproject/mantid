#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGMODEL_H_

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IEnggDiffGSASFittingModel {

public:
  virtual ~IEnggDiffGSASFittingModel() = default;

  virtual std::vector<std::pair<int, size_t>> getRunLabels() const = 0;

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
