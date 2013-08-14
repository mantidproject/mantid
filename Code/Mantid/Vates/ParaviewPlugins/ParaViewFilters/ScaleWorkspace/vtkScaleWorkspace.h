#ifndef _vtkScaleWorkspace_h
#define _vtkScaleWorkspace_h
#include "vtkUnstructuredGridAlgorithm.h"
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkScaleWorkspace : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkScaleWorkspace *New();
  vtkTypeMacro(vtkScaleWorkspace, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetXScaling(double xScaling);
  void SetYScaling(double yScaling);
  void SetZScaling(double zScaling);

protected:
  vtkScaleWorkspace();
  ~vtkScaleWorkspace();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkScaleWorkspace(const vtkScaleWorkspace&);
  void operator = (const vtkScaleWorkspace&);
  double m_xScaling;
  double m_yScaling;
  double m_zScaling;
};
#endif
