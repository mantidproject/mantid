#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "MantidVatesAPI/vtkPolyDataAlgorithm_Silent.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"

class vtkImplicitFunction;
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkNexusPeaksReader : public vtkPolyDataAlgorithm {
public:
  static vtkNexusPeaksReader *New();
  vtkNexusPeaksReader(const vtkNexusPeaksReader &) = delete;
  void operator=(const vtkNexusPeaksReader &) = delete;
  vtkTypeMacro(vtkNexusPeaksReader,
               vtkPolyDataAlgorithm) void PrintSelf(ostream &os,
                                                    vtkIndent indent) override;
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char *fname);
  void SetDimensions(int dimensions);
  /// Setter for the unitegrated peak marker size
  void SetUnintPeakMarkerSize(double mSize);
  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress, const std::string &message);
  /// Getter for the workspace type
  const std::string &GetWorkspaceTypeName();

protected:
  vtkNexusPeaksReader();
  ~vtkNexusPeaksReader() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  /// Handle time variation.
  vtkMTimeType GetMTime() override;

private:
  /// File name from which to read.
  char *FileName;

  /// Flag indicates when set up is complete wrt  the conversion of the nexus
  /// file to a MDEventWorkspace stored in ADS.
  bool m_isSetup;

  /// Cached PeaksWs Name
  std::string m_wsTypeName;

  /// Size for the unintegrated peak markers
  double m_uintPeakMarkerSize;

  /// Cached PeaksWS
  Mantid::API::IPeaksWorkspace_sptr m_PeakWS;

  /// Int representing an enum for q_lab, etc.
  int m_dimensions;
};
#endif
