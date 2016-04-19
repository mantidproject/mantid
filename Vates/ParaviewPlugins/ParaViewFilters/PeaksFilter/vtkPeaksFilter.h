#ifndef _VTKPEAKSFILTER_h
#define _VTKPEAKSFILTER_h
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include <boost/scoped_ptr.hpp>
#include <string>
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkPeaksFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPeaksFilter *New();
  vtkTypeMacro(vtkPeaksFilter, vtkUnstructuredGridAlgorithm) void PrintSelf(
      ostream &os, vtkIndent indent) override;
  void SetPeaksWorkspace(std::string peaksWorkspaceName);
  void SetRadiusNoShape(double radius);
  void SetRadiusType(Mantid::Geometry::PeakShape::RadiusType type);
  void SetDelimiter(std::string delimiter);
  void updateAlgorithmProgress(double progress, const std::string& message);
  double GetMinValue();
  double GetMaxValue();
  const char* GetInstrument();
protected:
  vtkPeaksFilter();
  ~vtkPeaksFilter() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  vtkPeaksFilter(const vtkPeaksFilter&);
  void operator = (const vtkPeaksFilter&);
  std::vector<std::string> extractPeakWorkspaceNames();
  std::vector<Mantid::API::IPeaksWorkspace_sptr>
  getPeaksWorkspaces(const std::vector<std::string> &peaksWorkspaceNames);
  std::string m_peaksWorkspaceNames;
  std::string m_delimiter;
  double m_radiusNoShape;
  Mantid::Geometry::PeakShape::RadiusType m_radiusType;
  double m_minValue;
  double m_maxValue;
  std::string m_instrument;
  boost::scoped_ptr<Mantid::VATES::MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<Mantid::VATES::VatesConfigurations> m_vatesConfigurations;
  int m_coordinateSystem;
};
#endif
