#ifndef _vtkSplatterPlot_h
#define _vtkSplatterPlot_h

#include "MantidKernel/make_unique.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include <string>

namespace Mantid {
namespace VATES {
class vtkSplatterPlotFactory;
}
}

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkSplatterPlot : public vtkUnstructuredGridAlgorithm {
public:
  static vtkSplatterPlot *New();
  vtkSplatterPlot(const vtkSplatterPlot &) = delete;
  void operator=(const vtkSplatterPlot &) = delete;
  // clang-format off
  vtkTypeMacro(vtkSplatterPlot, vtkUnstructuredGridAlgorithm)
  double getTime() const;
  // clang-format on
  void PrintSelf(ostream &os, vtkIndent indent) override;
  void SetNumberOfPoints(int nPoints);
  void SetTopPercentile(double topPercentile);
  void updateAlgorithmProgress(double progress, const std::string &message);
  /// Getter for the maximum value of the workspace data
  const char *GetInstrument();

protected:
  vtkSplatterPlot();
  ~vtkSplatterPlot() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  /// Number of total points to plot
  size_t m_numberPoints;
  /// Percent of densest boxes to keep
  double m_topPercentile;
  /// MVP presenter
  std::unique_ptr<Mantid::VATES::vtkSplatterPlotFactory> m_presenter;
  /// Holder for the workspace name
  std::string m_wsName;
  /// Time.
  double m_time;
};
#endif
