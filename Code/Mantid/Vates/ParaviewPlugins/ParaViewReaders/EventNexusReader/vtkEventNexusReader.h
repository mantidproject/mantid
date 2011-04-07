#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "vtkStructuredGridAlgorithm.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"

class VTK_EXPORT vtkEventNexusReader : public vtkStructuredGridAlgorithm
{
public:
  static vtkEventNexusReader *New();
  vtkTypeRevisionMacro(vtkEventNexusReader,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);

protected:
  vtkEventNexusReader();
  ~vtkEventNexusReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  
private:
  vtkEventNexusReader(const vtkEventNexusReader&);
  void operator = (const vtkEventNexusReader&);
  char *FileName;
  Mantid::VATES::MultiDimensionalDbPresenter m_presenter;
};
#endif
