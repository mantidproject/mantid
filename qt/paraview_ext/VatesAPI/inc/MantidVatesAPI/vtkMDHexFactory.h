// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTK_MD_HEX_FACTORY_H_
#define MANTID_VATES_VTK_MD_HEX_FACTORY_H_

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <vtkNew.h>

namespace Mantid {
namespace VATES {

/// Round up to next multiple of factor
coord_t DLLExport roundUp(const coord_t num_to_round, const coord_t factor);

/// Round down to previous multiple of factor
coord_t DLLExport roundDown(const coord_t num_to_round, const coord_t factor);

/** Class is used to generate vtkUnstructuredGrids from IMDEventWorkspaces.
Utilises the non-uniform nature of the underlying workspace grid/box structure
as the basis for generating visualisation cells. The recursion depth through the
box structure is configurable.

 @author Owen Arnold, Tessella plc
 @date 27/July/2011
 */

class DLLExport vtkMDHexFactory : public vtkDataSetFactory {

public:
  /// Constructor
  vtkMDHexFactory(const VisualNormalization normalizationOption,
                  const size_t maxDepth = 1000);

  /// Destructor
  ~vtkMDHexFactory() override;

  /// Factory Method. Should also handle delegation to successors.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdate) const override;

  /// Initalize with a target workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Get the name of the type.
  std::string getFactoryTypeName() const override { return "vtkMDHexFactory"; }

  void setRecursionDepth(size_t depth) override;

  /// Set the time value.
  void setTime(double timeStep);

private:
  coord_t
  getNextBinBoundary(const Mantid::API::IMDEventWorkspace_sptr &imdws) const;

  coord_t getPreviousBinBoundary(
      const Mantid::API::IMDEventWorkspace_sptr &imdws) const;

  template <typename MDE, size_t nd>
  void doCreate(
      typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) const;

  /// Template Method pattern to validate the factory before use.
  void validate() const override;

  /// Normalization option and info.
  const VisualNormalization m_normalizationOption;

  /// Member workspace to generate vtkdataset from.
  Mantid::API::Workspace_sptr m_workspace;

  /// Maximum recursion depth to use.
  size_t m_maxDepth;

  /// Data set that will be generated
  mutable vtkSmartPointer<vtkDataSet> dataSet;

  /// We are slicing down from > 3 dimensions
  mutable bool slice;

  /// Mask for choosing along which dimensions to slice
  mutable std::unique_ptr<bool[]> sliceMask;

  /// Implicit function to define which boxes to render.
  mutable boost::shared_ptr<Mantid::Geometry::MDImplicitFunction>
      sliceImplicitFunction;

  /// Time value.
  double m_time;
};
} // namespace VATES
} // namespace Mantid

#endif
