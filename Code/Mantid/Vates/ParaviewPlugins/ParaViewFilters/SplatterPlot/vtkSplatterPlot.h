#ifndef _vtkSplatterPlot_h
#define _vtkSplatterPlot_h
#include "vtkUnstructuredGridAlgorithm.h"
#include <string>
class VTK_EXPORT vtkSplatterPlot : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkSplatterPlot *New();
  vtkTypeMacro(vtkSplatterPlot, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetNumberOfPoints(int nPoints);
  void SetTopPercentile(double topPercentile);
  void updateAlgorithmProgress(double progress, const std::string& message);
protected:

  vtkSplatterPlot();
  ~vtkSplatterPlot();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkSplatterPlot(const vtkSplatterPlot&);
  void operator = (const vtkSplatterPlot&);
  size_t m_numberPoints;
  double m_topPercentile;
};
#endif
