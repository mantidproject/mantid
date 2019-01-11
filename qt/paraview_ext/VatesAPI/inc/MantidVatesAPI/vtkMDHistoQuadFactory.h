// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTK_MD_HISTO_QUAD_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_QUAD_FACTORY_H_

#include "MantidKernel/System.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "vtkUnstructuredGrid.h"

namespace Mantid {
namespace VATES {

/** Quad Factory. This type is responsible for rendering IMDWorkspaces as
surfaces with two spatial dimensions. Ideally this would be done as a polydata
type rather than a unstructuredgrid type,
however, some visualisation frameworks won't be able to treat these factories in
a covarient fashion.

 @author Owen Arnold, Tessella plc
 @date 03/05/2011
 */
class DLLExport vtkMDHistoQuadFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkMDHistoQuadFactory(const VisualNormalization normalizationOption);

  /// Assignment operator
  vtkMDHistoQuadFactory &operator=(const vtkMDHistoQuadFactory &other);

  /// Copy constructor.
  vtkMDHistoQuadFactory(const vtkMDHistoQuadFactory &other);

  /// Destructor
  ~vtkMDHistoQuadFactory() override;

  /// Factory Method.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  using Plane = std::vector<std::vector<UnstructuredPoint>>;

  using Column = std::vector<UnstructuredPoint>;

  std::string getFactoryTypeName() const override {
    return "vtkMDHistoQuadFactory";
  }

protected:
  void validate() const override;

private:
  Mantid::DataObjects::MDHistoWorkspace_sptr m_workspace;

  VisualNormalization m_normalizationOption;
};
} // namespace VATES
} // namespace Mantid
#endif
