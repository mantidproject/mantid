// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATES_MD_LINE_FACTORY
#define VATES_MD_LINE_FACTORY

#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace VATES {

/** Factory for creating a vtkDataSet from an IMDEventWorkspace with a single
 non-integrated dimensions.
 Delegates processing to a successor if these conditions are not met.

 @date 2012-02-10
*/
class DLLExport vtkMDLineFactory : public vtkDataSetFactory {

public:
  /// Constructor
  vtkMDLineFactory(const VisualNormalization normalizationOption);

  /// Destructor
  ~vtkMDLineFactory() override;

  /// Factory Method. Should also handle delegation to successors.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  /// Initalize with a target workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Get the name of the type.
  std::string getFactoryTypeName() const override;

protected:
  /// Template Method pattern to validate the factory before use.
  void validate() const override;

private:
  /// Name of the scalar.
  const VisualNormalization m_normalizationOption;

  /// Data source for the visualisation.
  Mantid::API::Workspace_sptr m_workspace;
};
} // namespace VATES
} // namespace Mantid

#endif
