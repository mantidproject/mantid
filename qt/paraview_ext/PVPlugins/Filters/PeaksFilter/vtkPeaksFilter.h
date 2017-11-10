#ifndef _VTKPEAKSFILTER_h
#define _VTKPEAKSFILTER_h
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include <boost/scoped_ptr.hpp>
#include <string>
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkPeaksFilter : public vtkUnstructuredGridAlgorithm {
public:
  static vtkPeaksFilter *New();
  vtkTypeMacro(vtkPeaksFilter, vtkUnstructuredGridAlgorithm) void PrintSelf(
      ostream &os, vtkIndent indent) override;
  vtkPeaksFilter(const vtkPeaksFilter &) = delete;
  vtkPeaksFilter &operator=(const vtkPeaksFilter &) = delete;
  void SetPeaksWorkspace(const std::string &peaksWorkspaceName,
                         const std::string &delimiter);
  void SetRadiusNoShape(double radius);
  void SetRadiusType(int type);
  void updateAlgorithmProgress(double progress, const std::string &message);
  const char *GetInstrument();

protected:
  vtkPeaksFilter();
  ~vtkPeaksFilter() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  std::vector<Mantid::API::IPeaksWorkspace_sptr>
  getPeaksWorkspaces(const Mantid::Kernel::StringTokenizer &workspaceNames);
  double m_radiusNoShape;
  int m_coordinateSystem;
  Mantid::Geometry::PeakShape::RadiusType m_radiusType;
  std::string m_instrument;
  std::vector<Mantid::API::IPeaksWorkspace_sptr> m_peaksWorkspaces;
  boost::scoped_ptr<Mantid::VATES::MetadataJsonManager> m_metadataJsonManager;
  boost::scoped_ptr<Mantid::VATES::VatesConfigurations> m_vatesConfigurations;
};
#endif
