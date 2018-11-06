// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTK_MD_HISTO_HEX_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_HEX_FACTORY_H_

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid.
 Uses Thresholding technique
 * to create sparse 3D representation of data.

 @author Owen Arnold, Tessella plc
 @date 06/05/2011
 */

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>

namespace Mantid {
namespace VATES {

class DLLExport vtkMDHistoHexFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkMDHistoHexFactory(const VisualNormalization normalizationOption);

  /// Assignment operator
  vtkMDHistoHexFactory &operator=(const vtkMDHistoHexFactory &other);

  /// Copy constructor.
  vtkMDHistoHexFactory(const vtkMDHistoHexFactory &other);

  /// Destructor
  ~vtkMDHistoHexFactory() override;

  /// Initialize the object with a workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Factory method
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  std::string getFactoryTypeName() const override {
    return "vtkMDHistoHexFactory";
  }

protected:
  void validate() const override;

  vtkSmartPointer<vtkDataSet> create3Dor4D(size_t timestep,
                                           ProgressAction &update) const;

  void validateWsNotNull() const;

  void validateDimensionsPresent() const;

  /// Image from which to draw.
  Mantid::DataObjects::MDHistoWorkspace_sptr m_workspace;

  /// Normalization option
  VisualNormalization m_normalizationOption;
};
} // namespace VATES
} // namespace Mantid

#endif
