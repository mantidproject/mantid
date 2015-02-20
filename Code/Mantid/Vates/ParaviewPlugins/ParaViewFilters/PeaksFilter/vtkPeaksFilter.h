#ifndef _VTKPEAKSFILTER_h
#define _VTKPEAKSFILTER_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include <boost/scoped_ptr.hpp>
#include <string>
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkPeaksFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPeaksFilter *New();
  vtkTypeMacro(vtkPeaksFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetPeaksWorkspace(std::string peaksWorkspaceName);
  void SetRadiusNoShape(double radius);
  void SetRadiusType(int type);
protected:
  vtkPeaksFilter();
  ~vtkPeaksFilter();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkPeaksFilter(const vtkPeaksFilter&);
  void operator = (const vtkPeaksFilter&);
  std::string m_peaksWorkspaceName;
  double m_radiusNoShape;
  int m_radiusType;
  double m_minValue;
  double m_maxValue;
  std::string m_instrument;
  boost::scoped_ptr<Mantid::VATES::MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<Mantid::VATES::VatesConfigurations> m_vatesConfigurations;
};
#endif
