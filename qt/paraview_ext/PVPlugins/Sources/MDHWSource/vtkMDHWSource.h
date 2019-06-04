// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _vtkMDHWSource_h
#define _vtkMDHWSource_h


#include "MantidVatesAPI/Normalization.h"
#include "vtkStructuredGridAlgorithm.h"

#include <string>

namespace Mantid {
namespace VATES {
class MDLoadingPresenter;
}
} // namespace Mantid

/*  Source for fetching Multidimensional Workspace out of the Mantid Analysis
   Data Service
    and converting them into vtkDataSets as part of the pipeline source.

    @date 01/12/2011

*/

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkMDHWSource : public vtkStructuredGridAlgorithm {
public:
  vtkMDHWSource(const vtkMDHWSource &) = delete;
  void operator=(const vtkMDHWSource &) = delete;
  static vtkMDHWSource *New();
  vtkTypeMacro(vtkMDHWSource, vtkStructuredGridAlgorithm) void PrintSelf(
      ostream &os, vtkIndent indent) override;

  void SetWsName(const std::string &wsName);

  //------- MDLoadingView methods ----------------
  virtual double getTime() const;
  virtual size_t getRecursionDepth() const;
  virtual bool getLoadInMemory() const;
  //----------------------------------------------

  /// Update the algorithm progress.
  void updateAlgorithmProgress(double progress, const std::string &message);
  /// Getter for the input geometry xml
  std::string GetInputGeometryXML();
  /// Getter for the special coodinate value
  int GetSpecialCoordinates();
  /// Getter for the workspace name
  const std::string &GetWorkspaceName();
  /// Getter for the workspace type
  std::string GetWorkspaceTypeName();
  /// Getter for the maximum value of the workspace data
  std::string GetInstrument();
  /// Setter for the normalization
  void SetNormalization(int option);

protected:
  vtkMDHWSource();
  ~vtkMDHWSource() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  /// Name of the workspace.
  std::string m_wsName;

  /// Time.
  double m_time;

  /// MVP presenter.
  std::unique_ptr<Mantid::VATES::MDLoadingPresenter> m_presenter;

  /// Normalization Option
  Mantid::VATES::VisualNormalization m_normalizationOption;

  void setTimeRange(vtkInformationVector *outputVector);
};
#endif
