#ifndef _vtkSQWReader_h
#define _vtkSQWReader_h

#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "vtkStructuredGridAlgorithm.h"
class VTK_EXPORT vtkSQWReader : public vtkStructuredGridAlgorithm
{
public:
  static vtkSQWReader *New();
  vtkTypeRevisionMacro(vtkSQWReader,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);

protected:
  vtkSQWReader();
  ~vtkSQWReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkSQWReader(const vtkSQWReader&);
  void operator = (const vtkSQWReader&);
  Mantid::VATES::MultiDimensionalDbPresenter m_presenter;
  char *FileName;
};
#endif
