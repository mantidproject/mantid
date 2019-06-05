// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _vtkMDEWSource_h
#define _vtkMDEWSource_h

#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include <string>

namespace Mantid {
namespace VATES {
class MDLoadingPresenter;
}
} // namespace Mantid

/*  Source for fetching Multidimensional Workspace out of the Mantid Analysis
   Data Service
    and converting them into vtkDataSets as part of the pipeline source.

    @author Owen Arnold @ Tessella
    @date 08/09/2011

*/

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkMDEWSource : public vtkUnstructuredGridAlgorithm {
public:
  static vtkMDEWSource *New();
  vtkMDEWSource(const vtkMDEWSource &) = delete;
  void operator=(const vtkMDEWSource &) = delete;
  // clang-format off
  vtkTypeMacro(vtkMDEWSource, vtkUnstructuredGridAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) override;
  // clang-format on
  void SetWsName(const std::string &wsName);
  void SetDepth(int depth);
  void SetNormalization(int option);

  //------- MDLoadingView methods ----------------
  virtual double getTime() const;
  virtual size_t getRecursionDepth() const;
  virtual bool getLoadInMemory() const;
  //----------------------------------------------

  /// Update the algorithm progress.
  void updateAlgorithmProgress(double, const std::string &message);
  /// Getter for the input geometry xml
  std::string GetInputGeometryXML();
  /// Getter for the special coodinate value
  int GetSpecialCoordinates();
  /// Getter for the workspace name
  const std::string &GetWorkspaceName();
  /// Getter for the workspace type
  std::string GetWorkspaceTypeName();
  /// Getter for the instrument associated with the workspace data.
  std::string GetInstrument();

protected:
  vtkMDEWSource();
  ~vtkMDEWSource() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  /// Name of the workspace.
  std::string m_wsName;

  /// Recursion depth.
  size_t m_depth;

  /// Time.
  double m_time;

  /// MVP presenter.
  std::unique_ptr<Mantid::VATES::MDLoadingPresenter> m_presenter;

  /// Normalization option
  Mantid::VATES::VisualNormalization m_normalization;
  void setTimeRange(vtkInformationVector *outputVector);
};
#endif
