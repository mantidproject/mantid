#ifndef _vtkMDEWSplatterReader_h
#define _vtkMDEWSplatterReader_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidKernel/MultiThreaded.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkMDEWSplatterReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkMDEWSplatterReader *New();
  vtkTypeRevisionMacro(vtkMDEWSplatterReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetInMemory(bool inMemory);
  void SetNumberOfPoints(int depth);
  void updateAlgorithmProgress(double progress);

protected:
  vtkMDEWSplatterReader();
  ~vtkMDEWSplatterReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  
  vtkMDEWSplatterReader(const vtkMDEWSplatterReader&);
  
  void operator = (const vtkMDEWSplatterReader&);


  /// File name from which to read.
  char *FileName;

  /// The maximum threshold of counts for the visualisation.
  double m_maxThreshold;

  /// The minimum threshold of counts for the visualisation.
  double m_minThreshold;

  /// Flag indicating that file loading algorithm should attempt to fully load the file into memory.
  bool m_loadInMemory;

  /// True if the data needs to be reloaded (due to a setting change).
  bool m_needsLoading;

  /// Threshold range strategy
  Mantid::VATES::ThresholdRange_scptr m_ThresholdRange;

  //Recursion depth.
  size_t m_numberPoints;

  /// Mutex for thread-safe progress reporting.
  Mantid::Kernel::Mutex progressMutex;
};
#endif
