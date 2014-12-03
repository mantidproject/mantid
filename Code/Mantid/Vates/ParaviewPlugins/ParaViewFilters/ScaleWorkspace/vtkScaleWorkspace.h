#ifndef _vtkScaleWorkspace_h
#define _vtkScaleWorkspace_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include <boost/scoped_ptr.hpp>

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
  /// Getter for the minimum value of the workspace data
  double GetMinValue();
  /// Getter for the maximum value of the workspace data
  double GetMaxValue();
  /// Getter for the instrument associated with the instrument.
  const char* GetInstrument();

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

  double m_minValue;
  double m_maxValue;
  std::string m_instrument;

  boost::scoped_ptr<Mantid::VATES::MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<Mantid::VATES::VatesConfigurations> m_vatesConfigurations;
};
#endif
