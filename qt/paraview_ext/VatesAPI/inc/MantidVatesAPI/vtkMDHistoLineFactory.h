// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTK_MD_HISTO_LINE_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_LINE_FACTORY_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "vtkUnstructuredGrid.h"
#include <vtkNew.h>

namespace Mantid {
namespace VATES {

/** Line Factory. This type is responsible for rendering IMDWorkspaces as a
 single dimension.

 @author Owen Arnold, Tessella plc
 @date 09/05/2011
 */
class DLLExport vtkMDHistoLineFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkMDHistoLineFactory(const VisualNormalization normalizationOption);

  /// Assignment operator
  vtkMDHistoLineFactory &operator=(const vtkMDHistoLineFactory &other);

  /// Copy constructor.
  vtkMDHistoLineFactory(const vtkMDHistoLineFactory &other);

  /// Destructor
  ~vtkMDHistoLineFactory() override;

  /// Factory Method.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  using Column = std::vector<UnstructuredPoint>;

  std::string getFactoryTypeName() const override {
    return "vtkMDHistoLineFactory";
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
