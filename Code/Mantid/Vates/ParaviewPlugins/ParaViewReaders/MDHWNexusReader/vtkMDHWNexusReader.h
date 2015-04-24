#ifndef _vtkMDHWNexusReader_h
#define _vtkMDHWNexusReader_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/MDHWNexusLoadingPresenter.h"
#include "MantidKernel/MultiThreaded.h"

class vtkImplicitFunction;
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkMDHWNexusReader : public vtkUnstructuredGridAlgorithm
{
public:

  static vtkMDHWNexusReader *New();
  vtkTypeMacro(vtkMDHWNexusReader, vtkUnstructuredGridAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)
  int CanReadFile(const char* fname);
  void SetInMemory(bool inMemory);
  void SetDepth(int depth);
  
  //------- MDLoadingView methods ----------------
  virtual double getTime() const;
  virtual size_t getRecursionDepth() const;
  virtual bool getLoadInMemory() const;
  //----------------------------------------------

  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress, const std::string& message);

  /// Getter for the workspace type
  char* GetWorkspaceTypeName();
  /// Getter for the input geometry
  const char* GetInputGeometryXML();

protected:
  vtkMDHWNexusReader();
  ~vtkMDHWNexusReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:

  void setTimeRange(vtkInformationVector* outputVector);

  vtkMDHWNexusReader(const vtkMDHWNexusReader&);
  
  void operator = (const vtkMDHWNexusReader&);

  /// File name from which to read.
  char *FileName;

  /// Controller/Presenter.
  Mantid::VATES::MDHWNexusLoadingPresenter* m_presenter;

  /// Flag indicating that file loading algorithm should attempt to fully load the file into memory.
  bool m_loadInMemory;

  /// Mutex for thread-safe progress reporting.
  Mantid::Kernel::Mutex progressMutex;

  /// Recursion depth.
  size_t m_depth;

  /// Time.
  double m_time;

  //Cached workspace type name.
  std::string typeName;
};
#endif
