#ifndef _vtkMDEWNexusReaderII_h
#define _vtkMDEWNexusReaderII_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidKernel/MultiThreaded.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkMDEWNexusReaderII : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkMDEWNexusReaderII *New();
  vtkTypeRevisionMacro(vtkMDEWNexusReaderII,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetInMemory(bool inMemory);
  void SetRecursionDepth(int depth);
  void updateAlgorithmProgress(double progress);

protected:
  vtkMDEWNexusReaderII();
  ~vtkMDEWNexusReaderII();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  
  vtkMDEWNexusReaderII(const vtkMDEWNexusReaderII&);
  
  void operator = (const vtkMDEWNexusReaderII&);


  /// File name from which to read.
  char *FileName;

  /// The maximum threshold of counts for the visualisation.
  double m_maxThreshold;

  /// The minimum threshold of counts for the visualisation.
  double m_minThreshold;

  /// Flag indicating that file loading algorithm should attempt to fully load the file into memory.
  bool m_loadInMemory;

  /// Threshold range strategy
  Mantid::VATES::ThresholdRange_scptr m_ThresholdRange;

  //Recursion depth.
  size_t m_recursionDepth;

  /// Mutex for thread-safe progress reporting.
  Mantid::Kernel::Mutex progressMutex;
};
#endif
