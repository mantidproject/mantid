// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTK_MD_HISTO_HEX4D_FACTORY_H_
#define MANTID_VATES_VTK_MD_HISTO_HEX4D_FACTORY_H_

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid.
 Uses Thresholding technique
 * to create sparse 4D representation of data.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010
 */

#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkUnstructuredGrid.h>

namespace Mantid {
namespace VATES {

template <typename TimeMapper>
class DLLExport vtkMDHistoHex4DFactory : public vtkMDHistoHexFactory {
public:
  /// Constructor
  vtkMDHistoHex4DFactory(const VisualNormalization normalizationOption,
                         const double timestep);

  /// Assignment operator
  vtkMDHistoHex4DFactory &
  operator=(const vtkMDHistoHex4DFactory<TimeMapper> &other);

  /// Copy constructor.
  vtkMDHistoHex4DFactory(const vtkMDHistoHex4DFactory<TimeMapper> &other);

  /// Destructor
  ~vtkMDHistoHex4DFactory() override;

  /// Initialize the object with a workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Factory method
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  std::string getFactoryTypeName() const override {
    return "vtkMDHistoHex4DFactory";
  }

protected:
  void validate() const override;

private:
  using PointMap = std::vector<std::vector<std::vector<UnstructuredPoint>>>;
  using Plane = std::vector<std::vector<UnstructuredPoint>>;
  using Column = std::vector<UnstructuredPoint>;

  /// timestep obtained from framework.
  double m_timestep;

  /// Time mapper.
  TimeMapper m_timeMapper;
};
} // namespace VATES
} // namespace Mantid

#endif
