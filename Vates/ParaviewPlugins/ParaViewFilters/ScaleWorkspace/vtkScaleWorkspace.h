#ifndef _vtkScaleWorkspace_h
#define _vtkScaleWorkspace_h
#include "vtkPointSetAlgorithm.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include <boost/scoped_ptr.hpp>

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkScaleWorkspace : public vtkPointSetAlgorithm {
public:
  static vtkScaleWorkspace *New();
  vtkTypeMacro(vtkScaleWorkspace,
               vtkPointSetAlgorithm) void PrintSelf(ostream &os,
                                                    vtkIndent indent) override;
  void SetXScaling(double xScaling);
  void SetYScaling(double yScaling);
  void SetZScaling(double zScaling);
  double GetMinValue();
  double GetMaxValue();
  const char *GetInstrument();
  int GetSpecialCoordinates();

protected:
  vtkScaleWorkspace();
  ~vtkScaleWorkspace() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkScaleWorkspace(const vtkScaleWorkspace &);
  void operator=(const vtkScaleWorkspace &);
  void updateMetaData(vtkPointSet *inputDataSet);
  double m_xScaling;
  double m_yScaling;
  double m_zScaling;

  double m_minValue;
  double m_maxValue;
  std::string m_instrument;
  int m_specialCoordinates;

  boost::scoped_ptr<Mantid::VATES::MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<Mantid::VATES::VatesConfigurations> m_vatesConfigurations;
};
#endif
