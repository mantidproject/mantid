#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "MantidVatesAPI/vtkPolyDataAlgorithm_Silent.h"
#include "MantidAPI/IPeaksWorkspace.h"

class vtkImplicitFunction;
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkPeaksReader : public vtkPolyDataAlgorithm
{
public:
  static vtkPeaksReader *New();
  vtkTypeMacro(vtkPeaksReader, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)
  int CanReadFile(const char* fname);
  void SetDimensions(int dimensions);
  /// Setter for the unitegrated peak marker size
  void SetUnintPeakMarkerSize(double mSize);
  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress, const std::string& message);
  /// Getter for the workspace type
  char* GetWorkspaceTypeName();

protected:
  vtkPeaksReader();
  ~vtkPeaksReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  
  vtkPeaksReader(const vtkPeaksReader&);
  
  void operator = (const vtkPeaksReader&);

  /// File name from which to read.
  char *FileName;

  /// Flag indicates when set up is complete wrt  the conversion of the nexus file to a MDEventWorkspace stored in ADS.
  bool m_isSetup;
  
  /// Cached PeaksWs Name
  std::string m_wsTypeName;

  /// Size for the unintegrated peak markers
  double m_uintPeakMarkerSize;

  /// Cached PeaksWS
  Mantid::API::IPeaksWorkspace_sptr  m_PeakWS;

  /// Int representing an enum for q_lab, etc.
  int m_dimensions;

  

};
#endif
