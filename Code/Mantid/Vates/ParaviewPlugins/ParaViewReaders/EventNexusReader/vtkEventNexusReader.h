#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidMDAlgorithms/WidthParameter.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkEventNexusReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkEventNexusReader *New();
  vtkTypeRevisionMacro(vtkEventNexusReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetXBins(int x);
  void SetYBins(int y);
  void SetZBins(int z);
  void SetMaxThreshold(double maxThreshold);
  void SetMinThreshold(double minThreshold);
  void SetWidth(double width);
  void SetApplyClip(bool applyClip);
  void SetClipFunction( vtkImplicitFunction * func);

protected:
  vtkEventNexusReader();
  ~vtkEventNexusReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  vtkEventNexusReader(const vtkEventNexusReader&);
  void operator = (const vtkEventNexusReader&);
  char *FileName;
  Mantid::VATES::MultiDimensionalDbPresenter m_presenter;
  int m_nXBins;
  int m_nYBins;
  int m_nZBins;
  bool m_isSetup;
  double m_maxThreshold;
  double m_minThreshold;
  bool m_applyClip;
  vtkImplicitFunction* m_clipFunction;
  Mantid::MDAlgorithms::WidthParameter m_width;
  const std::string m_mdEventWsId;
  const std::string m_histogrammedWsId;
};
#endif
