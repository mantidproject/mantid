// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _vtkMDHWNexusReader_h
#define _vtkMDHWNexusReader_h


#include "MantidVatesAPI/MDHWNexusLoadingPresenter.h"
#include "MantidVatesAPI/Normalization.h"
#include "vtkStructuredGridAlgorithm.h"
#include <mutex>

class vtkImplicitFunction;
// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkMDHWNexusReader : public vtkStructuredGridAlgorithm {
public:
  static vtkMDHWNexusReader *New();
  vtkMDHWNexusReader(const vtkMDHWNexusReader &) = delete;
  void operator=(const vtkMDHWNexusReader &) = delete;
  vtkTypeMacro(vtkMDHWNexusReader, vtkStructuredGridAlgorithm) void PrintSelf(
      ostream &os, vtkIndent indent) override;
  vtkSetStringMacro(FileName)
      vtkGetStringMacro(FileName) int CanReadFile(const char *fname);
  void SetInMemory(bool inMemory);
  void SetDepth(int depth);

  //------- MDLoadingView methods ----------------
  virtual double getTime() const;
  virtual size_t getRecursionDepth() const;
  virtual bool getLoadInMemory() const;
  //----------------------------------------------

  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress, const std::string &message);

  /// Getter for the workspace type
  std::string GetWorkspaceTypeName();
  /// Getter for the input geometry
  std::string GetInputGeometryXML();
  /// Setter for the normalization
  void SetNormalization(int option);

protected:
  vtkMDHWNexusReader();
  ~vtkMDHWNexusReader() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  /// Handle time variation.
  vtkMTimeType GetMTime() override;

private:
  void setTimeRange(vtkInformationVector *outputVector);

  /// File name from which to read.
  char *FileName;

  /// Controller/Presenter.
  std::unique_ptr<Mantid::VATES::MDHWNexusLoadingPresenter> m_presenter;

  /// Flag indicating that file loading algorithm should attempt to fully load
  /// the file into memory.
  bool m_loadInMemory;

  /// Mutex for thread-safe progress reporting.
  std::mutex progressMutex;

  /// Recursion depth.
  size_t m_depth;

  /// Time.
  double m_time;

  /// Normalization Option
  Mantid::VATES::VisualNormalization m_normalizationOption;
};
#endif
