// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _vtkPeaksSource_h
#define _vtkPeaksSource_h

#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesAPI/vtkPolyDataAlgorithm_Silent.h"
#include <string>

/**
    Source for fetching Peaks Workspace out of the Mantid Analysis Data Service
    and converting them into vtkDataSets as part of the pipeline source.

    @date 06/10/2011

*/

// cppcheck-suppress class_X_Y
class VTK_EXPORT vtkPeaksSource : public vtkPolyDataAlgorithm {
public:
  static vtkPeaksSource *New();
  vtkPeaksSource(const vtkPeaksSource &) = delete;
  void operator=(const vtkPeaksSource &) = delete;
  // clang-format off
  vtkTypeMacro(vtkPeaksSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) override;
  // clang-format on
  void SetWsName(const std::string &wsName);
  void SetPeakDimension(int dim);
  /// Setter for the unitegrated peak marker size
  void SetUnintPeakMarkerSize(double mSize);
  /// Update the algorithm progress.
  void updateAlgorithmProgress(double progress, const std::string &message);
  /// Getter for the workspace name
  const std::string &GetWorkspaceName();
  /// Getter for the workspace type
  const std::string &GetWorkspaceTypeName();
  /// Getter for the instrument associated with the workspace
  const std::string &GetInstrument();

protected:
  vtkPeaksSource();
  ~vtkPeaksSource() override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  /// Name of the workspace.
  std::string m_wsName;

  /// Cache for the workspace type name
  std::string m_wsTypeName;

  /// Size for the unintegrated peak markers
  double m_uintPeakMarkerSize;

  /// View coodinate to show
  Mantid::VATES::vtkPeakMarkerFactory::ePeakDimensions m_dimToShow;

  /// Cached workspace.
  Mantid::API::IPeaksWorkspace_sptr m_PeakWS;

  /// Instrument name.
  std::string m_instrument;
};
#endif
