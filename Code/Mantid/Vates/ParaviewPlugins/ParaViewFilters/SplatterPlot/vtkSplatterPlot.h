#ifndef _vtkSplatterPlot_h
#define _vtkSplatterPlot_h
#include "vtkUnstructuredGridAlgorithm.h"
#include <string>
class VTK_EXPORT vtkSplatterPlot : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkSplatterPlot *New();
  vtkTypeRevisionMacro(vtkSplatterPlot,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetNumberOfPoints(int nPoints);
protected:

  vtkSplatterPlot();
  ~vtkSplatterPlot();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkSplatterPlot(const vtkSplatterPlot&);
  void operator = (const vtkSplatterPlot&);
  std::string findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id);
  size_t m_numberPoints;
};
#endif
