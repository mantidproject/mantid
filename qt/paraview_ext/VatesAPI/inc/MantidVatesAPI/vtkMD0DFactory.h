// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTK_MD_0D_FACTORY_H_
#define MANTID_VATES_VTK_MD_0D_FACTORY_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "vtkUnstructuredGrid.h"

namespace Mantid {
namespace VATES {

/** 0D Factory. This type is responsible for rendering IMDWorkspaces with 0D.
 */
class DLLExport vtkMD0DFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkMD0DFactory();

  /// Destructor
  ~vtkMD0DFactory() override;

  /// Factory Method.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  std::string getFactoryTypeName() const override { return "vtkMD0DFactory"; }

protected:
  void validate() const override;
};
} // namespace VATES
} // namespace Mantid
#endif
