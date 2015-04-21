#ifndef _vtkSQWEventReader_h
#define _vtkSQWEventReader_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/SQWLoadingPresenter.h"
#include "MantidKernel/MultiThreaded.h"

class vtkImplicitFunction;
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkSQWEventReader : public vtkUnstructuredGridAlgorithm
{
public:

   //------- MDLoadingView methods ----------------
  virtual double getTime() const;
  virtual size_t getRecursionDepth() const;
  virtual bool getLoadInMemory() const;
  //----------------------------------------------

  static vtkSQWEventReader *New();
  vtkTypeMacro(vtkSQWEventReader, vtkUnstructuredGridAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetInMemory(bool inMemory);
  void SetDepth(int depth);
  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress, const std::string& message);
  /// Getter for the workspace type
  char* GetWorkspaceTypeName();
  /// Getter for the input geometry
  const char* GetInputGeometryXML();

protected:
  vtkSQWEventReader();
  ~vtkSQWEventReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:

  vtkSQWEventReader(const vtkSQWEventReader&);
  
  void operator = (const vtkSQWEventReader&);

  void setTimeRange(vtkInformationVector* outputVector);

  /// File name from which to read.
  char *FileName;

  /// Controller/Presenter.
  Mantid::VATES::SQWLoadingPresenter* m_presenter;

  /// Flag indicating that file loading algorithm should attempt to fully load the file into memory.
  bool m_loadInMemory;

  /// Mutex for thread-safe progress reporting.
  Mantid::Kernel::Mutex progressMutex;

  /// Recursion depth.
  size_t m_depth;

  //Time
  double m_time;

  //Cached workspace type name.
  std::string typeName;
};
#endif
