#ifndef _vtkSplatterPlot_h
#define _vtkSplatterPlot_h

#include "vtkUnstructuredGridAlgorithm.h"
#include <string>

namespace Mantid
{
  namespace VATES
  {
    class vtkSplatterPlotFactory;
  }
}

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkSplatterPlot : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkSplatterPlot *New();
  vtkTypeMacro(vtkSplatterPlot, vtkUnstructuredGridAlgorithm)
  double getTime() const;
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetNumberOfPoints(int nPoints);
  void SetTopPercentile(double topPercentile);
  void updateAlgorithmProgress(double progress, const std::string& message);
  /// Getter for the minimum value of the workspace data
  double GetMinValue();
  /// Getter for the maximum value of the workspace data
  double GetMaxValue();
  /// Getter for the maximum value of the workspace data
  const char* GetInstrument();
protected:
  vtkSplatterPlot();
  ~vtkSplatterPlot();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  /// Number of total points to plot
  size_t m_numberPoints;
  /// Percent of densest boxes to keep
  double m_topPercentile;
  /// MVP presenter
  Mantid::VATES::vtkSplatterPlotFactory *m_presenter;
  /// Holder for the workspace name
  std::string m_wsName;
  /// Time.
  double m_time;

  vtkSplatterPlot(const vtkSplatterPlot&);
  void operator = (const vtkSplatterPlot&);
};
#endif
